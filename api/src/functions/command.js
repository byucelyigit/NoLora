const { app } = require('@azure/functions');
const { firebaseWrite, isKurudereAdmin } = require('../shared/firebase');

// 11..18 = röle ON, 21..28 = röle OFF
function isValidCommand(value) {
    return (
        Number.isInteger(value) &&
        ((value >= 11 && value <= 18) ||
            (value >= 21 && value <= 28) ||
            [-2, -3, -4, -5, -6].includes(value))
    );
}

app.http('command', {
    methods: ['PUT'],
    authLevel: 'anonymous',

    handler: async (request, context) => {
        if (!isKurudereAdmin(request)) {
            return { status: 403, jsonBody: { ok: false, error: 'Forbidden' } };
        }

        let command;

        try {
            ({ command } = await request.json());
        } catch {
            return {
                status: 400,
                jsonBody: { ok: false, error: 'Invalid JSON body' }
            };
        }

        if (!isValidCommand(command)) {
            return {
                status: 400,
                jsonBody: { ok: false, error: 'Invalid command' }
            };
        }

        try {
            await firebaseWrite('Params/Command', command);

            return { status: 200, jsonBody: { ok: true, command } };
        } catch (err) {
            context.error(err);
            return {
                status: 500,
                jsonBody: { ok: false, error: 'Unable to write command' }
            };
        }
    }
});
