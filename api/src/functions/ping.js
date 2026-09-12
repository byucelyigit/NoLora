const { app } = require('@azure/functions');
const crypto = require('crypto');

function secureCompare(a, b) {
    if (!a || !b) return false;

    const aBuffer = Buffer.from(a);
    const bBuffer = Buffer.from(b);

    if (aBuffer.length !== bBuffer.length) {
        return false;
    }

    return crypto.timingSafeEqual(aBuffer, bBuffer);
}

app.http('ping', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        const expectedKey = process.env.BRIDGE_API_KEY;

        if (!expectedKey) {
            context.error('BRIDGE_API_KEY is not configured');

            return {
                status: 500,
                jsonBody: {
                    ok: false,
                    error: 'Server configuration error'
                }
            };
        }

        const authHeader = request.headers.get('authorization');

        if (!authHeader || !authHeader.startsWith('Bearer ')) {
            return {
                status: 401,
                jsonBody: {
                    ok: false,
                    error: 'Unauthorized'
                }
            };
        }

        const suppliedKey = authHeader.substring(7);

        if (!secureCompare(suppliedKey, expectedKey)) {
            return {
                status: 403,
                jsonBody: {
                    ok: false,
                    error: 'Forbidden'
                }
            };
        }

        return {
            status: 200,
            jsonBody: {
                ok: true,
                message: 'Azure Managed Function authenticated'
            }
        };
    }
});