package mkalten.reactivision

import android.content.Context
import android.content.Context.MODE_PRIVATE

private const val PREFS_NAME = "reactivision_settings"
private const val IP_PREF = "preference_ip"

class Settings(val ctx: Context) {

    private val prefs get() = ctx.getSharedPreferences(PREFS_NAME, MODE_PRIVATE)

    var ip: String
        get() = prefs.getString(IP_PREF, "127.0.0.1") ?: "127.0.0.1"
        set(value) { prefs.edit().putString(IP_PREF, value).commit() }
}