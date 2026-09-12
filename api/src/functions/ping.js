const { app } = require('@azure/functions');
const crypto = require('crypto');

function secureCompare(a, b) {
    if (!a || !b) return false;

    const aBuffer = Buffer.from(a, 'utf8');
    const bBuffer = Buffer.from(b, 'utf8');

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

        // Kendi özel header'ımız
        const suppliedKey = request.headers.get('x-bridge-key');

        // Header hiç gönderilmemiş
        if (!suppliedKey) {
            return {
                status: 401,
                jsonBody: {
                    ok: false,
                    error: 'Unauthorized'
                }
            };
        }

        // Header var ama key yanlış
        if (!secureCompare(suppliedKey, expectedKey)) {
            return {
                status: 403,
                jsonBody: {
                    ok: false,
                    error: 'Forbidden'
                }
            };
        }

        // Başarılı
        return {
            status: 200,
            jsonBody: {
                ok: true,
                message: 'Azure Managed Function authenticated'
            }
        };
    }
});