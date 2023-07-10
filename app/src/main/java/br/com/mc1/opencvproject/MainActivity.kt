package br.com.mc1.opencvproject

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Bundle
import android.view.View
import android.widget.SeekBar
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import br.com.mc1.opencvproject.databinding.ActivityMainBinding
import java.lang.Float.max

class MainActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener {

    var srcBitmap: Bitmap? = null
    var dstBitmap: Bitmap? = null

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Load the original image
        srcBitmap = BitmapFactory.decodeResource(this.resources, R.drawable.mountain)

        // Create and display dstBitmap in image view, we will keep updating
        // dstBitmap and the changes will be displayed on screen
        dstBitmap = srcBitmap!!.copy(srcBitmap!!.config, true)
        this.binding.imageView.setImageBitmap(dstBitmap)

        this.binding.sldSigma.setOnSeekBarChangeListener(this)

    }

    private fun doBlur() {
        // The SeekBar range is 0-100 convert it to 0.1-10
        val sigma = max(0.1F, this.binding.sldSigma.progress / 10F)

        // This is the actual call to the blur method inside native-lib.cpp
        this.myBlurJNI(srcBitmap!!, dstBitmap!!, sigma)

        val isBlurred = this.blurDetectJNI(dstBitmap!!, 10.0)
        Toast.makeText(this, "IsBlurred? $isBlurred", Toast.LENGTH_SHORT).show()
    }

    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
        this.doBlur()
    }

    fun btnFlipClick(view: View) {
        // This is the actual call to the blur method inside native-lib.cpp
        // note we flip srcBitmap (which is not displayed) and then call doBlur which will
        // eventually update dstBitmap (and which is displayed)
        this.myFlipJNI(srcBitmap!!, srcBitmap!!)
        this.doBlur()
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {}
    override fun onStopTrackingTouch(seekBar: SeekBar?) {}

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    private external fun myFlipJNI(bitmamIn: Bitmap, bitmapOut: Bitmap)
    private external fun myBlurJNI(bitmamIn: Bitmap, bitmapOut: Bitmap, sigma: Float)
    private external fun stitchImagesJNI(bitmapsIn: Array<Bitmap>, bitmapsOut: Array<Bitmap>)

    private external fun blurDetectJNI(bitmamIn: Bitmap, threshold: Double): Boolean

    companion object {

        // ** IMPORTANT ** used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }

}