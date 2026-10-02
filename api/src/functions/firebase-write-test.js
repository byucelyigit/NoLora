const { app } = require('@azure/functions');
const crypto = require('crypto');
const {
    firebaseRead,
    firebaseWrite,
    firebaseDelete
} = require('../shared/firebase');

function secureCompare(a, b) {
    if (!a || !b) return false;

    const aBuffer = Buffer.from(a, 'utf8');
    const bBuffer = Buffer.from(b, 'utf8');

    if (aBuffer.length !== bBuffer.length) {
        return false;
    }

    return crypto.timingSafeEqual(aBuffer, bBuffer);
}

app.http('firebase-write-test', {
    methods: ['POST'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        const suppliedKey =
            request.headers.get('x-bridge-key');

        const expectedKey =
            process.env.BRIDGE_API_KEY;

        if (!suppliedKey) {
            return {
                status: 401,
                jsonBody: {
                    ok: false,
                    error: 'Unauthorized'
                }
            };
        }

        if (!secureCompare(suppliedKey, expectedKey)) {
            return {
                status: 403,
                jsonBody: {
                    ok: false,
                    error: 'Forbidden'
                }
            };
        }

        try {
            const path = 'Params/_bridge_test';

            const testData = {
                source: 'azure-bridge',
                test: true,
                timestamp: new Date().toISOString()
            };

            await firebaseWrite(path, testData);

            const readData = await firebaseRead(path);

            await firebaseDelete(path);

            return {
                status: 200,
                jsonBody: {
                    ok: true,
                    writeSuccessful: true,
                    readBack: readData,
                    cleanupSuccessful: true
                }
            };

        } catch (err) {
            context.error(err);

            return {
                status: 500,
                jsonBody: {
                    ok: false,
                    error: err.message
                }
            };
        }
    }
});