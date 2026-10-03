const { app } = require('@azure/functions');
const {
    firebaseRead,
    firebaseWrite,
    isKurudereAdmin
} = require('../shared/firebase');

app.http('pressure', {
    methods: ['GET', 'PUT'],
    authLevel: 'anonymous',

    handler: async (request, context) => {
        if (!isKurudereAdmin(request)) {
            return { status: 403, jsonBody: { ok: false, error: 'Forbidden' } };
        }

        if (request.method === 'GET') {
            try {
                const [current, defaultMinLimit, min, max, pressureLost] =
                    await Promise.all([
                        firebaseRead('Pressure/Current'),
                        firebaseRead('Pressure/DefaultMinLimit'),
                        firebaseRead('Pressure/Min'),
                        firebaseRead('Pressure/Max'),
                        firebaseRead('Pressure/PressureLost')
                    ]);

                return {
                    status: 200,
                    headers: { 'Cache-Control': 'no-store' },
                    jsonBody: {
                        ok: true,
                        current,
                        defaultMinLimit,
                        min,
                        max,
                        pressureLost
                    }
                };
            } catch (err) {
                context.error(err);
                return {
                    status: 500,
                    jsonBody: { ok: false, error: 'Unable to read pressure data' }
                };
            }
        }

        let defaultMinLimit;
        try {
            ({ defaultMinLimit } = await request.json());
        } catch {
            return {
                status: 400,
                jsonBody: { ok: false, error: 'Invalid JSON body' }
            };
        }

        if (!Number.isSafeInteger(defaultMinLimit) || defaultMinLimit < 0) {
            return {
                status: 400,
                jsonBody: {
                    ok: false,
                    error: 'Minimum limit must be a non-negative integer'
                }
            };
        }

        try {
            await firebaseWrite('Pressure/DefaultMinLimit', defaultMinLimit);
            return {
                status: 200,
                jsonBody: { ok: true, defaultMinLimit }
            };
        } catch (err) {
            context.error(err);
            return {
                status: 500,
                jsonBody: { ok: false, error: 'Unable to write minimum limit' }
            };
        }
    }
});
