package ups.com.aplicacionnativa;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Base64;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

import ups.com.aplicacionnativa.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;
    private Bitmap bitmapI;
    private Bitmap bitmapO;
    private ImageView imgView;
    private ImageView imgViewProcesada;
    private static final int REQUEST_IMAGE_CAPTURE = 101;
    private static final String TAG = "MainActivity";

    static {
        System.loadLibrary("aplicacionnativa");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        initializeViews();

        Button btnCapturar = findViewById(R.id.Capturar);
        btnCapturar.setOnClickListener(v -> abrirCamara());

        Button btnEnviar = findViewById(R.id.enviar);
        btnEnviar.setOnClickListener(v -> {
            if (bitmapO != null) { // Enviar la imagen procesada
                sendImageToServer(bitmapO);
            } else {
                Log.e(TAG, "Bitmap procesada es Null");
            }
        });

        Button btnProcesamiento = findViewById(R.id.procesamiento);
        btnProcesamiento.setOnClickListener(v -> applyImageProcessing());
    }

    private void initializeViews() {
        imgView = findViewById(R.id.imageView);
        imgViewProcesada = findViewById(R.id.imageViewProcesada);
    }

    private void abrirCamara() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, REQUEST_IMAGE_CAPTURE);
        } else {
            dispatchTakePictureIntent();
        }
    }

    private void dispatchTakePictureIntent() {
        Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
        if (intent.resolveActivity(getPackageManager()) != null) {
            startActivityForResult(intent, REQUEST_IMAGE_CAPTURE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_IMAGE_CAPTURE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                abrirCamara();
            }
        }
    }

    private void applyImageProcessing() {
        if (bitmapI != null) {
            bitmapO = bitmapI.copy(bitmapI.getConfig(), true);
            eliminarChroma(bitmapO);
            imgViewProcesada.setImageBitmap(bitmapO);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_IMAGE_CAPTURE && resultCode == RESULT_OK) {
            Bundle extras = data.getExtras();
            if (extras != null) {
                Bitmap imageBitmap = (Bitmap) extras.get("data");
                if (imageBitmap != null) {
                    bitmapI = imageBitmap;
                    bitmapO = Bitmap.createBitmap(bitmapI.getWidth(), bitmapI.getHeight(), bitmapI.getConfig());
                    imgView.setImageBitmap(bitmapI);
                }
            }
        }
    }

    public native void eliminarChroma(Bitmap bitmapI);

    private String convertirBitmapABase64(Bitmap bitmap) {
        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
        bitmap.compress(Bitmap.CompressFormat.PNG, 100, byteArrayOutputStream);
        byte[] byteArray = byteArrayOutputStream.toByteArray();
        return Base64.encodeToString(byteArray, Base64.DEFAULT);
    }

    // Método para enviar la imagen al servidor
    private void sendImageToServer(Bitmap bitmap) {
        OkHttpClient client = new OkHttpClient();

        String base64Image = convertirBitmapABase64(bitmap);
        JSONObject jsonRequest = new JSONObject();
        try {
            jsonRequest.put("image", base64Image);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonString = jsonRequest.toString();
        Log.d(TAG, "JSON to send: " + jsonString);

        String serverUrl = "http://192.168.209.235:5000/upload";
        RequestBody body = RequestBody.create(jsonString, MediaType.get("application/json; charset=utf-8"));
        Request request = new Request.Builder()
                .url(serverUrl)
                .post(body)
                .build();

        client.newCall(request).enqueue(new okhttp3.Callback() {
            @Override
            public void onFailure(okhttp3.Call call, IOException e) {
                e.printStackTrace();
                Log.e(TAG, "Request failed: " + e.getMessage());
            }

            @Override
            public void onResponse(okhttp3.Call call, Response response) throws IOException {
                if (!response.isSuccessful()) {
                    throw new IOException("Unexpected code " + response);
                }
                String responseBody = response.body().string();
                Log.d(TAG, "Respuesta Server: " + responseBody);
            }
        });
    }

}
