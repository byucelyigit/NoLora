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
    const apiKey = process.env.FIREBASE_API_KEY;
    const email = process.env.FIREBASE_EMAIL;
    const password = process.env.FIREBASE_PASSWORD;

    if (!apiKey || !email || !password) {
        throw new Error('Firebase authentication variables are missing');
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

async function readParams(idToken) {
    let dbUrl = process.env.FIREBASE_DB_URL;

    if (!dbUrl) {
        throw new Error('FIREBASE_DB_URL is missing');
    }

    // Sonda / varsa kaldır
    dbUrl = dbUrl.replace(/\/+$/, '');

    const url =
        `${dbUrl}/Params.json?auth=${encodeURIComponent(idToken)}`;

    const response = await fetch(url);

    const result = await response.json();

    if (!response.ok) {
        throw new Error(
            `Firebase read failed: ${JSON.stringify(result)}`
        );
    }

    return result;
}

app.http('firebase-read-params', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        // 1. Azure Bridge anahtarını kontrol et
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
            // 2. Firebase'e bridge kullanıcısıyla giriş yap
            const auth = await firebaseLogin();

            // 3. Params dalını oku
            const params = await readParams(auth.idToken);

            return {
                status: 200,
                jsonBody: {
                    ok: true,
                    uid: auth.localId,
                    path: 'Params',
                    data: params
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