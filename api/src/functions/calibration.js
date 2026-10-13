const { app } = require('@azure/functions');
const {
    firebaseDelete,
    firebaseRead,
    firebaseWrite,
    isKurudereAdmin
} = require('../shared/firebase');

const RELAY_COUNT = 4;
const MAX_SAMPLES = 300;
const REFERENCE_RUN_COUNT = 4;
const CALIBRATION_PATH = 'Pressure/Calibration';
const COMMAND_CALIBRATION = -8;

function parseLine(value) {
    const line = Number(value);
    return Number.isInteger(line) && line >= 1 && line <= RELAY_COUNT
        ? line
        : null;
}

function runsPath(line) {
    return `Pressure/Calibrations/Line_${line}/Runs`;
}

app.http('calibration', {
    methods: ['GET', 'POST', 'DELETE'],
    authLevel: 'anonymous',

    handler: async (request, context) => {
        if (!isKurudereAdmin(request)) {
            return { status: 403, jsonBody: { ok: false, error: 'Forbidden' } };
        }

        if (request.method === 'GET') {
            const line = parseLine(request.query.get('line'));
            if (line === null) {
                return {
                    status: 400,
                    jsonBody: { ok: false, error: 'Line must be between 1 and 4' }
                };
            }

            try {
                const [runs, status] = await Promise.all([
                    firebaseRead(runsPath(line)),
                    firebaseRead(`${CALIBRATION_PATH}/Status`)
                ]);

                return {
                    status: 200,
                    headers: { 'Cache-Control': 'no-store' },
                    jsonBody: {
                        ok: true,
                        line,
                        runs: runs && typeof runs === 'object' ? runs : {},
                        status
                    }
                };
            } catch (err) {
                context.error(err);
                return {
                    status: 500,
                    jsonBody: { ok: false, error: 'Unable to read calibration data' }
                };
            }
        }

        if (request.method === 'DELETE') {
            const line = parseLine(request.query.get('line'));
            if (line === null) {
                return {
                    status: 400,
                    jsonBody: { ok: false, error: 'Line must be between 1 and 4' }
                };
            }

            try {
                const status = await firebaseRead(`${CALIBRATION_PATH}/Status`);
                if (status && ['queued', 'running', 'uploading'].includes(status.state)) {
                    return {
                        status: 409,
                        jsonBody: { ok: false, error: 'Cannot delete data during calibration' }
                    };
                }

                const runs = await firebaseRead(runsPath(line));
                const runIds = runs && typeof runs === 'object' && !Array.isArray(runs)
                    ? Object.keys(runs).sort()
                    : [];
                const referenceRunIds = runIds.slice(0, REFERENCE_RUN_COUNT);
                const removableRunIds = runIds.slice(REFERENCE_RUN_COUNT);

                await Promise.all(removableRunIds.map(runId =>
                    firebaseDelete(`${runsPath(line)}/${runId}`)
                ));

                return {
                    status: 200,
                    jsonBody: {
                        ok: true,
                        line,
                        preserved: referenceRunIds.length,
                        deleted: removableRunIds.length
                    }
                };
            } catch (err) {
                context.error(err);
                return {
                    status: 500,
                    jsonBody: { ok: false, error: 'Unable to delete calibration data' }
                };
            }
        }

        let body;
        try {
            body = await request.json();
        } catch {
            return {
                status: 400,
                jsonBody: { ok: false, error: 'Invalid JSON body' }
            };
        }

        if (!body || typeof body !== 'object' || Array.isArray(body)) {
            return {
                status: 400,
                jsonBody: { ok: false, error: 'Invalid JSON body' }
            };
        }

        const line = parseLine(body.line);
        const durationMinutes = body.durationMinutes;
        const intervalSeconds = body.intervalSeconds;
        if (
            line === null ||
            !Number.isSafeInteger(durationMinutes) ||
            durationMinutes < 1 ||
            durationMinutes > 1440 ||
            !Number.isSafeInteger(intervalSeconds) ||
            intervalSeconds < 1 ||
            intervalSeconds > 86400
        ) {
            return {
                status: 400,
                jsonBody: {
                    ok: false,
                    error: 'Invalid line, duration or sample interval'
                }
            };
        }

        const requestedSampleCount = Math.floor(durationMinutes * 60 / intervalSeconds);
        if (requestedSampleCount < 1) {
            return {
                status: 400,
                jsonBody: {
                    ok: false,
                    error: 'Duration must be at least one sample interval'
                }
            };
        }
        const sampleCount = Math.min(requestedSampleCount, MAX_SAMPLES);

        try {
            const status = await firebaseRead(`${CALIBRATION_PATH}/Status`);
            if (status && ['queued', 'running', 'uploading'].includes(status.state)) {
                return {
                    status: 409,
                    jsonBody: { ok: false, error: 'A calibration is already active' }
                };
            }

            await firebaseWrite('Params/n', line);
            await firebaseWrite('Params/x', durationMinutes);
            await firebaseWrite('Params/y', intervalSeconds);
            await firebaseWrite(`${CALIBRATION_PATH}/Status`, {
                state: 'queued',
                line,
                durationMinutes,
                intervalSeconds,
                sampleCount,
                message: 'Kalibrasyon isteği cihaza iletildi'
            });
            await firebaseWrite('Params/Command', COMMAND_CALIBRATION);

            return {
                status: 202,
                jsonBody: {
                    ok: true,
                    line,
                    durationMinutes,
                    intervalSeconds,
                    sampleCount
                }
            };
        } catch (err) {
            context.error(err);
            return {
                status: 500,
                jsonBody: { ok: false, error: 'Unable to start calibration' }
            };
        }
    }
});
