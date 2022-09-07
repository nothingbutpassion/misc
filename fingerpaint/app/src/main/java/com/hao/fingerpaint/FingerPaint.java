package com.hao.fingerpaint;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import java.io.FileNotFoundException;
import java.io.InputStream;

public class FingerPaint extends Activity {

    private static final int COLOR_MENU_ID = Menu.FIRST;
//    private static final int EMBOSS_MENU_ID = Menu.FIRST + 1;
//    private static final int BLUR_MENU_ID = Menu.FIRST + 2;
    private static final int SPACE_MENU_ID = Menu.FIRST + 3;
    private static final int ENTER_MENU_ID = Menu.FIRST + 4;
    private static final int SAVE_MENU_ID = Menu.FIRST + 5;
    private static final int IMAGE_MENU_ID = Menu.FIRST + 6;
    private MyView mMyView;


    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mMyView = new MyView(this);
        setContentView(mMyView);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.add(0, COLOR_MENU_ID, 0, "Color");
//        menu.add(0, EMBOSS_MENU_ID, 0, "Emboss");
//        menu.add(0, BLUR_MENU_ID, 0, "Blur");
        menu.add(0, SPACE_MENU_ID, 0, "Space");
        menu.add(0, ENTER_MENU_ID, 0, "Enter");
        menu.add(0, SAVE_MENU_ID, 0, "Save");
        menu.add(0, IMAGE_MENU_ID, 0, "Image");
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case COLOR_MENU_ID:
                mMyView.selectColor();
                return true;
//            case EMBOSS_MENU_ID:
//                mMyView.emboss();
//                return true;
//            case BLUR_MENU_ID:
//                mMyView.blur();
//                return true;
            case SPACE_MENU_ID:
                mMyView.addSpace();
                return true;
            case ENTER_MENU_ID:
                mMyView.addEnter();
                return true;
            case SAVE_MENU_ID:
                mMyView.save();
                return true;
            case IMAGE_MENU_ID:
                Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                intent.setType("image/*");
                startActivityForResult(intent, IMAGE_MENU_ID);
                return true;
        }

        return super.onOptionsItemSelected(item);
    }


    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == IMAGE_MENU_ID && resultCode == RESULT_OK) {
            Uri uri = data.getData();
            try {
                InputStream inputStream = getContentResolver().openInputStream(uri);
                Bitmap bitmap = BitmapFactory.decodeStream(inputStream);
                mMyView.setBackgroud(bitmap);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
}
