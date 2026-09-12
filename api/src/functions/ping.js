const { app } = require('@azure/functions');

app.http('ping', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {
        return {
            status: 200,
            jsonBody: {
                ok: true,
                message: 'Azure Managed Function calisiyor'
            }
        };
    }
});