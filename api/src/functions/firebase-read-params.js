const { app } = require('@azure/functions');
const crypto = require('crypto');
const { firebaseRead } = require('../shared/firebase');

function secureCompare(a, b) {
    if (!a || !b) return false;

    const aBuffer = Buffer.from(a, 'utf8');
    const bBuffer = Buffer.from(b, 'utf8');

    if (aBuffer.length !== bBuffer.length) {
        return false;
    }

    return crypto.timingSafeEqual(aBuffer, bBuffer);
}

app.http('firebase-read-params', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        // 1. Azure Bridge anahtarını kontrol et
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
            // 2. Params dalını oku
            const params = await firebaseRead('Params');

            return {
                status: 200,
                jsonBody: {
                    ok: true,
                    path: 'Params',
                    data: params
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