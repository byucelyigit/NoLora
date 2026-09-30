const { app } = require('@azure/functions');

async function firebaseLogin() {
    const apiKey = process.env.FIREBASE_API_KEY;
    const email = process.env.FIREBASE_EMAIL;
    const password = process.env.FIREBASE_PASSWORD;

    if (!apiKey || !email || !password) {
        throw new Error('Firebase environment variables are missing');
    }

    const response = await fetch(
        `https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=${apiKey}`,
        {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                email,
                password,
                returnSecureToken: true
            })
        }
    );

    const result = await response.json();

    if (!response.ok) {
        throw new Error(
            `Firebase login failed: ${JSON.stringify(result)}`
        );
    }

    return result;
}

app.http('firebase-test', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        // Önce bizim Azure bridge key kontrolümüz
        const bridgeKey = request.headers.get('x-bridge-key');
        const expectedBridgeKey = process.env.BRIDGE_API_KEY;

        if (!bridgeKey || bridgeKey !== expectedBridgeKey) {
            return {
                status: 401,
                jsonBody: {
                    ok: false,
                    error: 'Unauthorized'
                }
            };
        }

        try {
            const auth = await firebaseLogin();

            return {
                status: 200,
                jsonBody: {
                    ok: true,
                    firebaseAuthenticated: true,
                    uid: auth.localId,
                    email: auth.email,
                    expiresIn: auth.expiresIn
                }
            };

        } catch (err) {
            context.error(err);

            return {
                status: 500,
                jsonBody: {
                    ok: false,
                    error: err.message
                }
            };
        }
    }
});