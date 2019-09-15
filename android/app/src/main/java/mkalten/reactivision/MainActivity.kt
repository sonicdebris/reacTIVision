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
import android.view.View.GONE
import android.view.View.VISIBLE
import android.view.WindowManager
import android.widget.EditText
import androidx.appcompat.app.AlertDialog
import kotlinx.android.synthetic.main.activity_main.*

private const val REQUEST_CODE_PERMISSIONS = 10
private val REQUIRED_PERMISSIONS = arrayOf(Manifest.permission.CAMERA)

class MainActivity : AppCompatActivity() {

    private val analyzer = ReactivisionAnalyzer(this@MainActivity)
    private val settings = Settings(this)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setContentView(R.layout.activity_main)

        if (allPermissionsGranted()) {
            preview.post { startCamera() }
        } else {
            ActivityCompat.requestPermissions(
                this, REQUIRED_PERMISSIONS, REQUEST_CODE_PERMISSIONS
            )
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            if (allPermissionsGranted()) {
               preview.post { startCamera() }
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

    private fun setupUi() {
        toggle_preview.setOnCheckedChangeListener { _, isChecked ->
            analyzer.engine?.showVideo(isChecked)
            preview.visibility = if (isChecked) VISIBLE else GONE
        }

        ip_button.setOnClickListener {
            showIpDialog()
        }
    }

    private fun showIpDialog() {

        val edit = EditText(this)

        AlertDialog.Builder(this)
            .setTitle("Set destination IP")
            .setView(edit)
            .setPositiveButton("Set") { _, _ ->
                val str = edit.text.toString()
                if (str.isNotBlank()) {
                    settings.ip = str
                    Toast.makeText(this, "Now restart the app", Toast.LENGTH_LONG).show()
                    finish()
                } else {
                    Toast.makeText(this, "Invalid IP", Toast.LENGTH_LONG).show()
                }
            }
            .setNegativeButton("Cancel", null).create().show()


    }

    private fun startCamera() {

        setupUi()

        // TODO: match the camera native aspect ratio / resolution
        //val metrics = DisplayMetrics().also { view_finder.display.getRealMetrics(it) }
        //val screenAspectRatio = Rational(metrics.widthPixels, metrics.heightPixels)

        val analyzerConfig = ImageAnalysisConfig.Builder().apply {
            // Use a worker thread for image analysis to prevent glitches
            val analyzerThread = HandlerThread("ReactivisionProcessing").apply { start() }
            setCallbackHandler(Handler(analyzerThread.looper))
            setImageReaderMode(ImageAnalysis.ImageReaderMode.ACQUIRE_LATEST_IMAGE)
            setTargetRotation(preview.display.rotation)
        }.build()

        // Build the image analysis use case and instantiate our analyzer
        val analysis = ImageAnalysis(analyzerConfig).apply {
            analyzer = this@MainActivity.analyzer
        }

        CameraX.bindToLifecycle(this as LifecycleOwner, analysis)
    }


    private class ReactivisionAnalyzer(val act: MainActivity) : ImageAnalysis.Analyzer {

        var engine: VisionEngine? = null
        var bmp: Bitmap? = null
        var pix: IntArray? = null

        var currentFps = .0
        var last = 0L

        override fun analyze(image: ImageProxy, rotationDegrees: Int) {

            val eng = engine ?: VisionEngine().apply {
                setup(image.width, image.height, act.settings.ip)
                showVideo(act.toggle_preview.isChecked)
                bmp = Bitmap.createBitmap(image.width, image.height, Bitmap.Config.ARGB_8888)
                pix = IntArray(image.width * image.height)
                act.preview.post { act.preview.setImageBitmap(bmp) }
                act.resolution.post { act.resolution.text = "${image.width} x ${image.height}" }
                engine = this
            }

            if (last == 0L) {
                last = System.nanoTime()
            } else {
                val now = System.nanoTime()
                val elapsed = now - last
                currentFps = 0.6 * currentFps + 0.4 * (1_000_000_000f / elapsed)
                last = now
                act.fps.post { act.fps.text = "%.1f fps".format(currentFps) }
            }

            // Format in ImageAnalysis is YUV: image.planes[0] contains the Y (luminance) plane
            val buffer = image.planes[0].buffer

            val show = eng.process(buffer)

            if (show) {
                pix?.let {
                    it.forEachIndexed { index, _ ->
                        val ibyte = buffer[index].toInt() and 0xFF
                        it[index] = 0x7F000000 or (ibyte shl 16) or (ibyte shl 8) or ibyte
                    }
                }

                bmp?.setPixels(pix, 0, image.width, 0, 0, image.width, image.height)
                act.preview.post { act.preview.invalidate() }
            }
        }
    }
}