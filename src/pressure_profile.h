#pragma once

#include <Arduino.h>
#include <math.h>

static const uint16_t PRESSURE_PROFILE_MAX_SAMPLES = 300;
static const uint8_t PRESSURE_PROFILE_FEATURE_COUNT = 5;

struct PressureProfileFeatures {
    float values[PRESSURE_PROFILE_FEATURE_COUNT];
};

struct PressureProfileAnalysis {
    float score;
    float threshold;
    bool anomalous;
};

inline float pressureProfileMedian(float* values, uint8_t count) {
    for (uint8_t i = 1; i < count; i++) {
        float value = values[i];
        int8_t j = static_cast<int8_t>(i) - 1;
        while (j >= 0 && values[j] > value) {
            values[j + 1] = values[j];
            j--;
        }
        values[j + 1] = value;
    }

    if (count % 2 == 1) {
        return values[count / 2];
    }
    return (values[count / 2 - 1] + values[count / 2]) / 2.0f;
}

inline PressureProfileFeatures pressureProfileExtractFeatures(
    const int* samples,
    uint16_t count
) {
    PressureProfileFeatures features = {};
    if (!samples || count == 0 || count > PRESSURE_PROFILE_MAX_SAMPLES) {
        return features;
    }

    int sorted[PRESSURE_PROFILE_MAX_SAMPLES];
    double sum = 0;
    double squareSum = 0;
    for (uint16_t i = 0; i < count; i++) {
        sorted[i] = samples[i];
        sum += samples[i];
        squareSum += static_cast<double>(samples[i]) * samples[i];
    }

    for (uint16_t i = 1; i < count; i++) {
        int value = sorted[i];
        int32_t j = static_cast<int32_t>(i) - 1;
        while (j >= 0 && sorted[j] > value) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = value;
    }

    double mean = sum / count;
    double variance = squareSum / count - mean * mean;
    if (variance < 0) {
        variance = 0;
    }

    features.values[0] = static_cast<float>(mean);
    features.values[1] = count % 2 == 1
        ? sorted[count / 2]
        : (sorted[count / 2 - 1] + sorted[count / 2]) / 2.0f;
    features.values[2] = sorted[(static_cast<uint32_t>(count - 1) * 9) / 10];
    features.values[3] = sorted[count - 1];
    features.values[4] = static_cast<float>(sqrt(variance));
    return features;
}

inline float pressureProfileScore(
    const PressureProfileFeatures& candidate,
    const PressureProfileFeatures* references,
    uint8_t referenceCount
) {
    static const float featureFloors[PRESSURE_PROFILE_FEATURE_COUNT] = {
        2.0f, 2.0f, 3.0f, 3.0f, 2.0f
    };

    float total = 0;
    for (uint8_t feature = 0; feature < PRESSURE_PROFILE_FEATURE_COUNT; feature++) {
        float values[4];
        for (uint8_t i = 0; i < referenceCount; i++) {
            values[i] = references[i].values[feature];
        }
        float center = pressureProfileMedian(values, referenceCount);

        float deviations[4];
        for (uint8_t i = 0; i < referenceCount; i++) {
            deviations[i] = fabsf(references[i].values[feature] - center);
        }
        float mad = pressureProfileMedian(deviations, referenceCount);
        float scale = fmaxf(featureFloors[feature], 1.4826f * mad);
        float deficit = center - candidate.values[feature];
        if (deficit > 0) {
            total += deficit / scale;
        }
    }
    return total / PRESSURE_PROFILE_FEATURE_COUNT;
}

inline PressureProfileAnalysis pressureProfileAnalyze(
    const int* const* referenceSamples,
    const uint16_t* referenceCounts,
    uint8_t referenceCount,
    const int* candidateSamples,
    uint16_t candidateCount
) {
    PressureProfileAnalysis result = {0, 0, false};
    if (!referenceSamples || !referenceCounts || referenceCount != 4 ||
        !candidateSamples || candidateCount < 20 ||
        candidateCount > PRESSURE_PROFILE_MAX_SAMPLES) {
        return result;
    }

    PressureProfileFeatures referenceFeatures[4];
    for (uint8_t i = 0; i < 4; i++) {
        if (!referenceSamples[i] || referenceCounts[i] != candidateCount) {
            return result;
        }
        referenceFeatures[i] = pressureProfileExtractFeatures(
            referenceSamples[i], referenceCounts[i]
        );
    }

    PressureProfileFeatures candidate = pressureProfileExtractFeatures(
        candidateSamples, candidateCount
    );
    float maximumHoldoutScore = 0;
    for (uint8_t heldOut = 0; heldOut < 4; heldOut++) {
        PressureProfileFeatures otherReferences[3];
        uint8_t next = 0;
        for (uint8_t i = 0; i < 4; i++) {
            if (i != heldOut) {
                otherReferences[next++] = referenceFeatures[i];
            }
        }

        float score = pressureProfileScore(
            referenceFeatures[heldOut], otherReferences, 3
        );
        if (score > maximumHoldoutScore) {
            maximumHoldoutScore = score;
        }
    }

    result.score = pressureProfileScore(candidate, referenceFeatures, 4);
    result.threshold = maximumHoldoutScore + 0.25f;
    result.anomalous = result.score > result.threshold;
    return result;
}
