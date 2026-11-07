package my.opengl.testsdl

import android.content.Intent
import android.os.Bundle
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import my.opengl.testsdl.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        ViewCompat.setOnApplyWindowInsetsListener(binding.root) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }

        binding.launchButton.setOnClickListener {
            startActivity(Intent(this, Maze3DActivity::class.java))
        }
        binding.buttonMatrix.setOnClickListener {
            startActivity(Intent(this, MatrixNatActivity::class.java))
        }
        binding.buttonMatrixSdlDelay.setOnClickListener {
            startActivity(Intent(this, MatrixNatSdlDelayActivity::class.java))
        }
        binding.buttonRaycasterPitch.setOnClickListener {
            startActivity(Intent(this, RaycasterPitchActivity::class.java))
        }
    }
}