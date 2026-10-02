let cachedToken = null;
let cachedExpiry = 0;

// Token bitiminden bu kadar önce yenile (ms)
const REFRESH_MARGIN_MS = 5 * 60 * 1000;

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
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email, password, returnSecureToken: true })
        }
    );

    const result = await response.json();

    if (!response.ok) {
        throw new Error(`Firebase login failed: ${JSON.stringify(result)}`);
    }

    return {
        idToken: result.idToken,
        expiresInMs: Number(result.expiresIn) * 1000
    };
}

async function getIdToken() {
    if (cachedToken && Date.now() < cachedExpiry - REFRESH_MARGIN_MS) {
        return cachedToken;
    }

    const { idToken, expiresInMs } = await firebaseLogin();
    cachedToken = idToken;
    cachedExpiry = Date.now() + expiresInMs;
    return cachedToken;
}

function firebaseUrl(path, idToken) {
    const dbUrl = process.env.FIREBASE_DB_URL;

    if (!dbUrl) {
        throw new Error('FIREBASE_DB_URL is missing');
    }

    return `${dbUrl.replace(/\/+$/, '')}/${path}.json?auth=${encodeURIComponent(idToken)}`;
}

async function firebaseRequest(method, path, value) {
    const idToken = await getIdToken();
    const options = { method };

    if (value !== undefined) {
        options.headers = { 'Content-Type': 'application/json' };
        options.body = JSON.stringify(value);
    }

    const response = await fetch(firebaseUrl(path, idToken), options);

    if (!response.ok) {
        const text = await response.text();
        throw new Error(`Firebase ${method} failed: ${text}`);
    }

    return response.json();
}

function firebaseRead(path) {
    return firebaseRequest('GET', path);
}

function firebaseWrite(path, value) {
    return firebaseRequest('PUT', path, value);
}

function firebaseDelete(path) {
    return firebaseRequest('DELETE', path);
}

// Route SWA tarafında da korunuyor; burada ikinci kontrol.
function isKurudereAdmin(request) {
    const encoded = request.headers.get('x-ms-client-principal');

    if (!encoded) {
        return false;
    }

    try {
        const principal = JSON.parse(
            Buffer.from(encoded, 'base64').toString('utf8')
        );

        return Array.isArray(principal.userRoles) &&
            principal.userRoles.includes('kurudere_admin');
    } catch {
        return false;
    }
}

module.exports = {
    getIdToken,
    firebaseRead,
    firebaseWrite,
    firebaseDelete,
    isKurudereAdmin
};
