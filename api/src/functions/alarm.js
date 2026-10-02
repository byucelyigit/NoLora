const { app } = require('@azure/functions');
const {
    firebaseRead,
    firebaseWrite,
    isKurudereAdmin
} = require('../shared/firebase');


/*
 * Yalnızca Alarm0, Alarm1, Alarm2...
 */
function validateAlarmNo(alarmNo) {

    return /^Alarm\d+$/.test(
        String(alarmNo || '')
    );
}


/*
 * Alarm0 => 0
 * Alarm1 => 1
 * ...
 *
 * Mevcut uygulamadaki Command
 * bitmask mantığını koruyoruz.
 */
function alarmNumber(alarmNo) {

    return parseInt(
        alarmNo.replace('Alarm', ''),
        10
    );
}


/*
 * Browser'dan gelen alanları
 * whitelist ile yeniden oluştur.
 *
 * Kullanıcı Firebase'e istediği
 * alanı yazamaz.
 */
function sanitizeAlarm(input) {

    const number = value => {

        const n = Number(value);

        if (!Number.isFinite(n)) {
            throw new Error(
                'Invalid numeric alarm value'
            );
        }

        return Math.trunc(n);
    };


    const alarm = {

        alarm_status:
            number(input.alarm_status),

        day_period:
            number(input.day_period),

        alarm_hour:
            number(input.alarm_hour),

        alarm_minute:
            number(input.alarm_minute),

        run_minutes:
            number(input.run_minutes),

        idle_minutes:
            number(input.idle_minutes),

        repeat_count:
            number(input.repeat_count),

        repeat_count_remaining:
            number(
                input.repeat_count_remaining
            ),

        run_remaining_minutes:
            number(
                input.run_remaining_minutes
            ),

        idle_remaining_minutes:
            number(
                input.idle_remaining_minutes
            ),

        last_run_date:
            String(
                input.last_run_date || ''
            ),

        relay_no:
            number(input.relay_no)
    };


    /*
     * Temel aralık kontrolleri
     */
    if (
        alarm.alarm_hour < 0 ||
        alarm.alarm_hour > 23
    ) {
        throw new Error(
            'alarm_hour must be 0-23'
        );
    }


    if (
        alarm.alarm_minute < 0 ||
        alarm.alarm_minute > 59
    ) {
        throw new Error(
            'alarm_minute must be 0-59'
        );
    }


    return alarm;
}


/*
 * İki alarm aynı mı?
 */
function alarmChanged(
    oldAlarm,
    newAlarm
) {

    return Object.keys(newAlarm)
        .some(
            key =>
                oldAlarm?.[key] !==
                newAlarm[key]
        );
}


/*
 * GET ve PUT
 */
app.http('alarm', {

    methods: [
        'GET',
        'PUT'
    ],

    authLevel: 'anonymous',

    handler: async (
        request,
        context
    ) => {

        /*
         * Azure role check
         */
        if (!isKurudereAdmin(request)) {

            return {
                status: 403,

                jsonBody: {
                    ok: false,
                    error: 'Forbidden'
                }
            };
        }


        const alarmNo =
            request.query.get(
                'alarmNo'
            );


        if (!validateAlarmNo(alarmNo)) {

            return {
                status: 400,

                jsonBody: {
                    ok: false,
                    error:
                        'Invalid alarm number'
                }
            };
        }


        try {

            /*
             * GET
             */
            if (
                request.method === 'GET'
            ) {

                const alarm =
                    await firebaseRead(
                        `Alarms/${alarmNo}`
                    );


                if (!alarm) {

                    return {
                        status: 404,

                        jsonBody: {
                            ok: false,
                            error:
                                'Alarm not found'
                        }
                    };
                }


                return {
                    status: 200,

                    headers: {
                        'Cache-Control':
                            'no-store'
                    },

                    jsonBody: {
                        ok: true,
                        alarmNo,
                        alarm
                    }
                };
            }


            /*
             * PUT
             */
            const input =
                await request.json();

            const updatedAlarm =
                sanitizeAlarm(input);


            const currentAlarm =
                await firebaseRead(
                    `Alarms/${alarmNo}`
                );


            if (!currentAlarm) {

                return {
                    status: 404,

                    jsonBody: {
                        ok: false,
                        error:
                            'Alarm not found'
                    }
                };
            }


            const changed =
                alarmChanged(
                    currentAlarm,
                    updatedAlarm
                );


            /*
             * Alarmı kaydet
             */
            await firebaseWrite(
                `Alarms/${alarmNo}`,
                updatedAlarm
            );


            /*
             * Mevcut Command bitmask
             * davranışını koru.
             */
            if (changed) {

                const no =
                    alarmNumber(alarmNo);

                if (
                    Number.isNaN(no) ||
                    no < 0
                ) {
                    throw new Error(
                        'Invalid alarm number'
                    );
                }


                const changeValue =
                    no === 0
                        ? 0
                        : (1 << (no - 1));


                let commandValue = 0;


                if (changeValue === 0) {

                    commandValue = 0;

                } else {

                    const currentCommand =
                        await firebaseRead(
                            'Params/Command'
                        );


                    const parsed =
                        parseInt(
                            currentCommand,
                            10
                        );


                    const existingMask =
                        Number.isNaN(parsed) ||
                        parsed < 0
                            ? 0
                            : parsed;


                    commandValue =
                        existingMask |
                        changeValue;
                }


                await firebaseWrite(
                    'Params/Command',
                    commandValue
                );
            }


            return {
                status: 200,

                jsonBody: {
                    ok: true,
                    alarmNo,
                    changed
                }
            };

        }
        catch (error) {

            context.error(
                'Alarm API failed',
                error
            );


            return {
                status: 500,

                jsonBody: {
                    ok: false,
                    error:
                        'Unable to process alarm'
                }
            };
        }
    }
});