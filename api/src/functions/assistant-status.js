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

app.http('assistant-status', {
    methods: ['GET'],
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
            const [pingtime, ip] = await Promise.all([
                firebaseRead('Params/pingtime'),
                firebaseRead('Params/ip')
            ]);

            return {
                status: 200,
                jsonBody: {
                    ok: true,
                    pingtime,
                    ip
                }
            };

        } catch (err) {
            context.error(err);

            return {
                status: 500,
                jsonBody: {
                    ok: false,
                    error: 'Unable to read Firebase status'
                }
            };
        }
    }
});