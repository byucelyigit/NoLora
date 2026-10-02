const { app } = require('@azure/functions');
const {
    firebaseRead,
    firebaseDelete,
    isKurudereAdmin
} = require('../shared/firebase');

const RELAY_COUNT = 8;

app.http('relay-logs', {
    methods: ['GET', 'DELETE'],
    authLevel: 'anonymous',

    handler: async (request, context) => {
        if (!isKurudereAdmin(request)) {
            return { status: 403, jsonBody: { ok: false, error: 'Forbidden' } };
        }

        try {
            if (request.method === 'DELETE') {
                await Promise.all(
                    Array.from({ length: RELAY_COUNT }, (_, i) =>
                        firebaseDelete(`RelayLogs/Relay${i}/Log`)
                    )
                );

                return { status: 200, jsonBody: { ok: true } };
            }

            const relayNoParam = request.query.get('relayNo');
            const relayNo = Number(relayNoParam);

            if (
                relayNoParam === null ||
                !Number.isInteger(relayNo) ||
                relayNo < 0 ||
                relayNo >= RELAY_COUNT
            ) {
                return {
                    status: 400,
                    jsonBody: { ok: false, error: 'Invalid relay number' }
                };
            }

            const log = await firebaseRead(`RelayLogs/Relay${relayNo}/Log`);

            return {
                status: 200,
                headers: { 'Cache-Control': 'no-store' },
                jsonBody: { ok: true, relayNo, log }
            };
        } catch (err) {
            context.error(err);
            return {
                status: 500,
                jsonBody: { ok: false, error: 'Relay log operation failed' }
            };
        }
    }
});
