const { app } = require('@azure/functions');
const { firebaseRead } = require('../shared/firebase');

app.http('firmware-version', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {
        try {
            const version = await firebaseRead('Params/FW_VERSION');

            return {
                status: 200,
                headers: {
                    'Content-Type': 'application/json',
                    'Cache-Control': 'no-store'
                },
                jsonBody: {
                    ok: true,
                    version
                }
            };
        } catch (err) {
            context.error(err);

            return {
                status: 500,
                jsonBody: {
                    ok: false,
                    error: 'Unable to read firmware version'
                }
            };
        }
    }
});
