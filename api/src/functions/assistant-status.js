const { app } = require('@azure/functions');
const crypto = require('crypto');

function secureCompare(a, b) {
    if (!a || !b) return false;

    const aBuffer = Buffer.from(a, 'utf8');
    const bBuffer = Buffer.from(b, 'utf8');

    if (aBuffer.length !== bBuffer.length) {
        return false;
    }

    return crypto.timingSafeEqual(aBuffer, bBuffer);
}

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
        throw new Error('Firebase read failed');
    }

    return result;
}

app.http('assistant-status', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        const suppliedKey =
            request.headers.get('x-bridge-key');

        const expectedKey =
            process.env.BRIDGE_API_KEY;

        if (!suppliedKey) {
            return {
                status: 401,
                jsonBody: {
                    ok: false,
                    error: 'Unauthorized'
                }
            };
        }

        if (!secureCompare(suppliedKey, expectedKey)) {
            return {
                status: 403,
                jsonBody: {
                    ok: false,
                    error: 'Forbidden'
                }
            };
        }

        try {
            const idToken = await firebaseLogin();

            const [pingtime, ip] = await Promise.all([
                firebaseRead('Params/pingtime', idToken),
                firebaseRead('Params/ip', idToken)
            ]);

            return {
                status: 200,
                jsonBody: {
                    ok: true,
                    pingtime,
                    ip
                }
            };

        } catch (err) {
            context.error(err);

            return {
                status: 500,
                jsonBody: {
                    ok: false,
                    error: 'Unable to read Firebase status'
                }
            };
        }
    }
});