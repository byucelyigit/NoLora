const { app } = require('@azure/functions');
const { firebaseRead } = require('../shared/firebase');


app.http('alarms', {

    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        try {

            const [alarms, relays] =
                await Promise.all([
                    firebaseRead('Alarms'),
                    firebaseRead('relays')
                ]);

            return {
                status: 200,

                headers: {
                    'Cache-Control': 'no-store'
                },

                jsonBody: {
                    ok: true,
                    alarms: alarms || {},
                    relays: relays || {}
                }
            };

        }
        catch (error) {

            context.error(
                'Alarms API error:',
                error
            );

            return {
                status: 500,

                headers: {
                    'Cache-Control': 'no-store'
                },

                jsonBody: {
                    ok: false,
                    error: 'Unable to read alarms'
                }
            };
        }
    }
});