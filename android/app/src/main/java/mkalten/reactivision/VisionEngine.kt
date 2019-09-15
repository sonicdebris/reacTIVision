package mkalten.reactivision

import java.nio.ByteBuffer

class VisionEngine {

    val peer: Long = initPeer()
    // NB: don't rename. native glue code relies on this name to access the property

    private external fun initPeer(): Long
    private external fun disposePeer(peer: Long)

    external fun setup(w: Int, h: Int, ip: String): Boolean
    external fun process(input: ByteBuffer): Boolean
    external fun showVideo(show: Boolean)

    protected fun finalize() {
        disposePeer(peer)
    }

    companion object {
        init {
            System.loadLibrary("reactivision-android")
        }
    }
}