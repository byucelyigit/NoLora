const { app } = require('@azure/functions');


async function firebaseLogin() {

    const response = await fetch(
        `https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=${process.env.FIREBASE_API_KEY}`,
        {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                email: process.env.FIREBASE_EMAIL,
                password: process.env.FIREBASE_PASSWORD,
                returnSecureToken: true
            })
        }
    );

    const result = await response.json();

    if (!response.ok) {
        throw new Error('Firebase login failed');
    }

    return result.idToken;
}


async function firebaseRead(path, idToken) {

    const dbUrl =
        process.env.FIREBASE_DB_URL.replace(/\/+$/, '');

    const response = await fetch(
        `${dbUrl}/${path}.json?auth=${encodeURIComponent(idToken)}`
    );

    const result = await response.json();

    if (!response.ok) {
        throw new Error(
            `Firebase read failed: ${path}`
        );
    }

    return result;
}


app.http('alarms', {

    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        try {

            const idToken =
                await firebaseLogin();

            const [alarms, relays] =
                await Promise.all([
                    firebaseRead('Alarms', idToken),
                    firebaseRead('relays', idToken)
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