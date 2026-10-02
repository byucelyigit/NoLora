const { app } = require('@azure/functions');
const { firebaseRead } = require('../shared/firebase');


app.http('status', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        try {

            // Sadece izin verdiğimiz iki alanı oku
            const [pingtime, ip] = await Promise.all([
                firebaseRead('Params/pingtime'),
                firebaseRead('Params/ip')
            ]);

            return {
                status: 200,
                headers: {
                    'Content-Type': 'application/json',
                    'Cache-Control': 'no-store'
                },
                jsonBody: {
                    ok: true,
                    pingtime: pingtime,
                    ip: ip
                }
            };

        } catch (err) {

            context.error(err);

            return {
                status: 500,
                jsonBody: {
                    ok: false,
                    error: 'Unable to read status'
                }
            };
        }
    }
});