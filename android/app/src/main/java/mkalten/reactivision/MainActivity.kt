package mkalten.reactivision

import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.CameraX
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageAnalysisConfig
import androidx.camera.core.ImageProxy
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner

import android.Manifest
import android.view.WindowManager
import kotlinx.android.synthetic.main.activity_main.*

private const val REQUEST_CODE_PERMISSIONS = 10
private val REQUIRED_PERMISSIONS = arrayOf(Manifest.permission.CAMERA)

class MainActivity : AppCompatActivity() {

    val engine = VisionEngine()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setContentView(R.layout.activity_main)

        if (allPermissionsGranted()) {
            result.post { startCamera() }
        } else {
            ActivityCompat.requestPermissions(
                this, REQUIRED_PERMISSIONS, REQUEST_CODE_PERMISSIONS
            )
        }

        engine.setup()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            if (allPermissionsGranted()) {
               result.post { startCamera() }
            } else {
                Toast.makeText(this, "Permissions not granted!", Toast.LENGTH_SHORT).show()
                finish()
            }
        }
    }

    private fun allPermissionsGranted() = REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(
            baseContext,
            it
        ) == PackageManager.PERMISSION_GRANTED
    }

    private fun startCamera() {

        // TODO: match the camera native aspect ratio / resolution
        //val metrics = DisplayMetrics().also { view_finder.display.getRealMetrics(it) }
        //val screenAspectRatio = Rational(metrics.widthPixels, metrics.heightPixels)

        val analyzerConfig = ImageAnalysisConfig.Builder().apply {
            // Use a worker thread for image analysis to prevent glitches
            val analyzerThread = HandlerThread("ReactivisionProcessing").apply { start() }
            setCallbackHandler(Handler(analyzerThread.looper))
            setImageReaderMode(ImageAnalysis.ImageReaderMode.ACQUIRE_LATEST_IMAGE)
            setTargetRotation(result.display.rotation)
        }.build()

        // Build the image analysis use case and instantiate our analyzer
        val analysis = ImageAnalysis(analyzerConfig).apply {
            analyzer = ReactivisionAnalyzer(this@MainActivity)
        }

        CameraX.bindToLifecycle(this as LifecycleOwner, analysis)
    }


    private class ReactivisionAnalyzer(val act: MainActivity) : ImageAnalysis.Analyzer {

        var bmp: Bitmap? = null
        var pix: IntArray? = null

        override fun analyze(image: ImageProxy, rotationDegrees: Int) {

            if (bmp == null) {
                bmp = Bitmap.createBitmap( image.width, image.height, Bitmap.Config.ARGB_8888)
                pix = IntArray(image.width * image.height)
                act.result.post { act.result.setImageBitmap(bmp) }
                act.sample_text.post { act.sample_text.text = "${image.width} x ${image.height}" }
            }

            // Format in ImageAnalysis is YUV: image.planes[0] contains the Y (luminance) plane
            val buffer = image.planes[0].buffer

            act.engine.process(buffer)

            pix?.let { it.forEachIndexed { index, _ ->
                val ibyte = buffer[index].toInt() and 0xFF
                it[index] = 0x7F000000 or (ibyte shl 16) or (ibyte shl 8) or ibyte
            }}

            bmp?.setPixels(pix, 0, image.width, 0, 0, image.width, image.height)
            act.result.post { act.result.invalidate() }
        }
    }
}