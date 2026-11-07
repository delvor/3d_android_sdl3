package my.opengl.testsdl

import org.libsdl.app.SDLActivity

class MatrixNatSdlDelayActivity : SDLActivity() {
    override fun getLibraries(): Array<String> = arrayOf("matrix_sdl_delay")
}