package com.ylm.myapplication

import android.content.Context
import android.content.res.AssetManager
import android.graphics.Canvas
import android.util.AttributeSet
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.ylm.myapplication.MainActivity.Companion.model
import org.pytorch.IValue
import org.pytorch.Tensor
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.nio.FloatBuffer

class VulkanPoseDetectionSurfaceView : SurfaceView, SurfaceHolder.Callback2, Runnable {
    /**
     * Time per frame for 60 FPS
     */
    private val MAX_FRAME_TIME = (1000.0 / 60.0).toInt()
    private var drawingActive = false
    private var mInputTensorBuffer = Tensor.allocateFloatBuffer(3 * 256 * 256)
    private var drawThread: Thread? = null
    init {
        System.loadLibrary("myapplication")
    }

    private external fun nativeCreate(surface: Surface, assetManager: AssetManager)
    private external fun nativeDestroy()
    private external fun nativeResize(width: Int, height: Int)
    private external fun nativeDraw()

    private external fun workWithFloatBuffer(buffer : FloatBuffer)
    private external fun heatmapJNI(heatmap: FloatArray)

    constructor(context: Context) : super(context) {
    }

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {
    }

    constructor(context: Context, attrs: AttributeSet, defStyle: Int) : super(
        context,
        attrs,
        defStyle
    ) {
    }

    constructor(context: Context, attrs: AttributeSet, defStyle: Int, defStyleRes: Int) : super(
        context,
        attrs,
        defStyle,
        defStyleRes
    ) {
    }

    init {
        alpha = 1F
        holder.addCallback(this)
    }

    override fun surfaceChanged(p0: SurfaceHolder, format: Int, width: Int, height: Int) {
        nativeResize(width, height)
    }

    override fun surfaceDestroyed(p0: SurfaceHolder) {
        drawingActive = false
        drawThread?.join(5000);
        nativeDestroy()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        holder.let { h ->
            nativeCreate(h.surface, resources.assets)
        }

        drawThread = Thread(this, "Draw thread")
        drawingActive = true
        drawThread!!.start()
    }

    override fun surfaceRedrawNeeded(p0: SurfaceHolder) {
        workWithFloatBuffer(mInputTensorBuffer)

        var mInputTensor = Tensor.fromBlob(
            mInputTensorBuffer,
            longArrayOf(1, 3, 256, 256)
        )

        // running the model
        val outputTensor = model.forward(IValue.from(mInputTensor)).toTensor()
        val heatmap = outputTensor.dataAsFloatArray
        heatmapJNI(heatmap)
        nativeDraw()
    }

    /**
     * Copies specified asset to the file in /files app directory and returns this file absolute path.
     *
     * @return absolute file path
     */
    @Throws(IOException::class)
    fun assetFilePath(context: Context, assetName: String): String? {
        val file = File(context.filesDir, assetName)
        if (file.exists() && file.length() > 0) {
            return file.absolutePath
        }
        context.assets.open(assetName).use { `is` ->
            FileOutputStream(file).use { os ->
                val buffer = ByteArray(4 * 1024)
                var read: Int
                while (`is`.read(buffer).also { read = it } != -1) {
                    os.write(buffer, 0, read)
                }
                os.flush()
            }
            return file.absolutePath
        }
    }

    override fun run() {
        while (drawingActive) {
            var frameStartTime = System.nanoTime()
            /*
            workWithFloatBuffer(mInputTensorBuffer)

            var mInputTensor = Tensor.fromBlob(
                mInputTensorBuffer,
                longArrayOf(1, 3, 256, 256)
            )

            // running the model
            val outputTensor = model.forward(IValue.from(mInputTensor)).toTensor()
            val heatmap = outputTensor.dataAsFloatArray
            heatmapJNI(heatmap)
            */

            nativeDraw()
            //var canvas = holder.lockCanvas(null);
            //if(canvas != null) {
            //    holder.unlockCanvasAndPost(canvas);
            //}

            // calculate the time required to draw the frame in ms
            var frameTime = (System.nanoTime() - frameStartTime) / 1000000
            Log.v("t", frameTime.toString());

            if (frameTime < MAX_FRAME_TIME) // faster than the max fps - limit the FPS
            {
                Thread.sleep(MAX_FRAME_TIME - frameTime)
            }
            else
            {
                Thread.sleep(1)
            }
        }
    }
}