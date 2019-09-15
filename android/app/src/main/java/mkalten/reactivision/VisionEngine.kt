package mkalten.reactivision

import java.nio.ByteBuffer

class VisionEngine {

    external fun setup(): Boolean
    external fun process(input: ByteBuffer)

    companion object {
        init {
            System.loadLibrary("reactivision-android")
        }
    }
}