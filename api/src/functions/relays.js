const { app } = require('@azure/functions');
const { firebaseRead, isKurudereAdmin } = require('../shared/firebase');

const RELAY_COUNT = 8;

app.http('relays', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {
        if (!isKurudereAdmin(request)) {
            return { status: 403, jsonBody: { ok: false, error: 'Forbidden' } };
        }

        try {
            const names = await Promise.all(
                Array.from({ length: RELAY_COUNT }, (_, i) =>
                    firebaseRead(`relays/relay${i}`)
                )
            );

            return {
                status: 200,
                headers: { 'Cache-Control': 'no-store' },
                jsonBody: { ok: true, names }
            };
        } catch (err) {
            context.error(err);
            return {
                status: 500,
                jsonBody: { ok: false, error: 'Unable to read relays' }
            };
        }
    }
});
