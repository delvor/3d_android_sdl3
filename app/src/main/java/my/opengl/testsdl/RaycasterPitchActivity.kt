package my.opengl.testsdl

import android.annotation.SuppressLint
import android.content.pm.ActivityInfo
import android.os.Bundle
import android.os.PersistableBundle
import org.libsdl.app.SDLActivity

class RaycasterPitchActivity : SDLActivity() {

    @SuppressLint("SourceLockedOrientationActivity")
    override fun onCreate(savedInstanceState: Bundle?, persistentState: PersistableBundle?) {
        super.onCreate(savedInstanceState, persistentState)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
    }
    override fun getLibraries(): Array<String> = arrayOf("raycaster_pitch")
}