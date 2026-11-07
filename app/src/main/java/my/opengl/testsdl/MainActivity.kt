package my.opengl.testsdl;


import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import androidx.appcompat.app.AppCompatActivity;


public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button gameButton = findViewById(R.id.launchButton);
        Button matrixButton = findViewById(R.id.button_matrix);
        Button matrixSdlDelayButton = findViewById(R.id.button_matrix_sdl_delay);

        gameButton.setOnClickListener(v -> startActivity(new Intent(MainActivity.this, Maze3DActivity.class)));

        matrixButton.setOnClickListener(v -> startActivity(new Intent(MainActivity.this, MatrixNatActivity.class)));
        matrixSdlDelayButton.setOnClickListener((view -> startActivity(new Intent(MainActivity.this, MatrixNatSdlDelayActivity.class))));
    }
}
