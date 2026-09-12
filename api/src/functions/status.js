const { app } = require('@azure/functions');

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

    return result.idToken;
}


async function firebaseRead(path, idToken) {

    let dbUrl = process.env.FIREBASE_DB_URL;

    if (!dbUrl) {
        throw new Error('FIREBASE_DB_URL is missing');
    }

    dbUrl = dbUrl.replace(/\/+$/, '');

    const url =
        `${dbUrl}/${path}.json?auth=${encodeURIComponent(idToken)}`;

    const response = await fetch(url);

    const result = await response.json();

    if (!response.ok) {
        throw new Error(
            `Firebase read failed: ${JSON.stringify(result)}`
        );
    }

    return result;
}


app.http('status', {
    methods: ['GET'],
    authLevel: 'anonymous',

    handler: async (request, context) => {

        try {

            // Firebase'e bridge kullanıcısıyla giriş yap
            const idToken = await firebaseLogin();

            // Sadece izin verdiğimiz iki alanı oku
            const [pingtime, ip] = await Promise.all([
                firebaseRead('Params/pingtime', idToken),
                firebaseRead('Params/ip', idToken)
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