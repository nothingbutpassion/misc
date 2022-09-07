package com.hao.fingerpaint;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;


public class MyView extends View {


    private static final long DELAY_TIME = 1000;
    private static final float TEXT_HIGHT = 120;
    private static final int BACK_COLOR = 0xFFFFFFFF;
    private static final int LINE_COLOR = 0xFFEEEEEE;


    private Character mCurrentCharacter;


    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (System.currentTimeMillis() - mLastTouchTime > DELAY_TIME/2) {
                if (mCurrentCharacter != null) {
                    DataManager.getInstance().addCharacter(mCurrentCharacter);
                    mCurrentCharacter = null;
                    mCharacterBitmap = Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.ARGB_8888);
                    mCharacterCanvas = new Canvas(mCharacterBitmap);
                    invalidate();
                }
            }
        }
    };

    ColorPickerDialog.OnColorChangedListener mColorChangedListener = new ColorPickerDialog.OnColorChangedListener() {
        @Override
        public void colorChanged(int color) {
            Paint paint = new Paint(mCharacterPaint);
            paint.setColor(color);
            mCharacterPaint = paint;
        }
    };

    private Bitmap mBackgroundBitmap;   // The background bitmap
    private Bitmap mContentBitmap;      // The content bitmap
    private Canvas mContentCanvas;      // The content canvas

    private Bitmap mCharacterBitmap;    // The current character bitmap
    private Canvas mCharacterCanvas;    // The current character canvas

    private Paint mBitmapPaint;         // Used to draw all kinds of bitmaps
    private Paint mCharacterPaint;      // Used to draw the character (or the path)
    private Path mCharacterPath;        // The path is a part of one charater


    public MyView(Context c) {
        super(c);

        mBitmapPaint = new Paint(Paint.DITHER_FLAG);
        mCharacterPath = new Path();
        mCharacterPaint = new Paint();
        mCharacterPaint.setAntiAlias(true);
        mCharacterPaint.setDither(true);
        mCharacterPaint.setColor(0xFF0000FF);
        mCharacterPaint.setStyle(Paint.Style.STROKE);
        mCharacterPaint.setStrokeJoin(Paint.Join.ROUND);
        mCharacterPaint.setStrokeCap(Paint.Cap.ROUND);
        mCharacterPaint.setStrokeWidth(24);

        setClickable(true);
        setFocusableInTouchMode(true);
    }

    public void save() {

        try {
            SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
            File path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
            File file = new File(path, df.format(new Date()) + ".png");
            path.mkdirs();

            // Draw what we wanted on the bitmap
            Bitmap bitmap = Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            drawBackground(canvas);
            drawContent(canvas, false);

            // Generate png file from bitmap
            bitmap.compress(Bitmap.CompressFormat.PNG, 0, new FileOutputStream(file));

            // Tell the media scanner about the new file so that it is
            // immediately available to the user.
            MediaScannerConnection.scanFile(getContext(),
                    new String[]{file.toString()}, null,
                    new MediaScannerConnection.OnScanCompletedListener() {
                        public void onScanCompleted(String path, Uri uri) {
                            Log.i("ExternalStorage", "Scanned " + path + ":");
                            Log.i("ExternalStorage", "-> uri=" + uri);
                        }
                    });

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
    }


    void selectColor() {
        new ColorPickerDialog(getContext(), mColorChangedListener, mCharacterPaint.getColor()).show();
    }

    void setBackgroud(Bitmap bitmap) {
        mBackgroundBitmap = bitmap;
        invalidate();
    }


    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        mCharacterBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        mCharacterCanvas = new Canvas(mCharacterBitmap);

        mContentBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        mContentCanvas = new Canvas(mContentBitmap);

        mCurrentCharacter = null;
    }


    private void drawBackground(Canvas canvas) {
        if (mBackgroundBitmap == null) {
            canvas.drawColor(BACK_COLOR);
        } else {
            int bw = mBackgroundBitmap.getWidth();
            int bh = mBackgroundBitmap.getHeight();
            int w = getWidth();
            int h = getHeight();

            Rect sr = new Rect(0, 0, bw, bh);
            RectF dr = new RectF();
            if (h*bw > w*bh) {
                dr.right = w;
                dr.bottom = w*bh/(float)bw;
            } else {
                dr.bottom = h;
                dr.right = w*bw/(float)bh;
            }

            canvas.drawBitmap(mBackgroundBitmap, sr, dr, mBitmapPaint);
        }
    }


    private void drawLines(Canvas canvas) {
        Paint paint = new Paint();
        paint.setColor(LINE_COLOR);
        paint.setStrokeWidth(2);
        int w = getWidth();
        int h = getHeight();
        for (int i = 1; i < h / TEXT_HIGHT; i++) {
            canvas.drawLine(4, i * TEXT_HIGHT, w - 4, i * TEXT_HIGHT, paint);
        }
    }

    private void drawContent(Canvas canvas) {
        drawContent(canvas, true);
    }

    private void drawContent(Canvas canvas, boolean drawCusor) {

        ArrayList<Character> characters = DataManager.getInstance().getCharacters();


        int w = getWidth();
        int h = getHeight();

        mCharacterBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        mCharacterCanvas.setBitmap(mCharacterBitmap);
        mContentBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        mContentCanvas.setBitmap(mContentBitmap);

        float factor = TEXT_HIGHT / h;
        int x = 0;
        int y = 0;
        for (Character c : characters) {
            switch (c.getType()) {
                case Character.TYPE_ENTER: {
                    x = 0;
                    y += TEXT_HIGHT;
                }
                break;

                case Character.TYPE_SPACE: {
                    float spaceWidth = w < h ? w * factor : h * factor;
                    if (x + spaceWidth > w) {
                        x = 0;
                        y += TEXT_HIGHT;
                    }
                    x += spaceWidth;
                }
                break;

                case Character.TYPE_PATH: {
                    RectF rect = c.getOriginalBound();
                    float padding = factor * w / 10.0f;
                    float actualWidth = factor * rect.width() + padding;
                    float widthOffset = padding - factor * rect.left;
                    if (x + actualWidth > w) {
                        x = 0;
                        y += TEXT_HIGHT;
                    }
                    mContentCanvas.save();
                    mContentCanvas.translate(x + widthOffset, y);
                    mContentCanvas.scale(factor, factor);
                    ArrayList<Path> paths = c.getPaths();
                    for (Path path : paths) {
                        mContentCanvas.drawPath(path, c.getPaint());
                    }
                    mContentCanvas.restore();
                    x += actualWidth;
                }
                break;
            }
        }

        // Draw cusor
        mContentCanvas.save();
        mContentCanvas.translate(x, y);
        mContentCanvas.scale(factor, factor);
        mContentCanvas.drawLine(w/4, h/8, w/4, 7*h/8, mCharacterPaint);
        mContentCanvas.restore();

        canvas.drawBitmap(mContentBitmap, 0, 0, mBitmapPaint);
    }

    private void drawCurrentCharacter(Canvas canvas) {
        if (mCurrentCharacter != null) {
            ArrayList<Path> paths = mCurrentCharacter.getPaths();
            for (Path path : paths) {
                mCharacterCanvas.drawPath(path, mCharacterPaint);
            }
            canvas.drawBitmap(mCharacterBitmap, 0, 0, mBitmapPaint);
            canvas.drawPath(mCharacterPath, mCharacterPaint);
        }
    }


    @Override
    protected void onDraw(Canvas canvas) {
        drawBackground(canvas);
        drawLines(canvas);
        drawContent(canvas);
        drawCurrentCharacter(canvas);
    }

    private float mX, mY;
    private long mLastTouchTime;
    private static final float TOUCH_TOLERANCE = 4;

    private void touch_start(float x, float y) {
        mCharacterPath.reset();
        mCharacterPath.moveTo(x, y);
        mX = x;
        mY = y;
        if (mCurrentCharacter == null) {
            mCurrentCharacter = new Character();
            mCurrentCharacter.setPaint(mCharacterPaint);
        }
        mLastTouchTime = System.currentTimeMillis();
    }

    private void touch_move(float x, float y) {
        float dx = Math.abs(x - mX);
        float dy = Math.abs(y - mY);
        if (dx >= TOUCH_TOLERANCE || dy >= TOUCH_TOLERANCE) {
            mCharacterPath.quadTo(mX, mY, (x + mX) / 2, (y + mY) / 2);
            mX = x;
            mY = y;
        }

        mLastTouchTime = System.currentTimeMillis();
    }

    private void touch_up(float x, float y) {
        mCharacterPath.lineTo(mX, mY);
        mCurrentCharacter.addPath(new Path(mCharacterPath));
        mCharacterPath.reset();

        mLastTouchTime = System.currentTimeMillis();
        mHandler.sendEmptyMessageDelayed(0, DELAY_TIME);
    }


    public void addEnter() {
        DataManager.getInstance().addCharacter(new Character(Character.TYPE_ENTER));
        invalidate();
    }

    public void addSpace() {
        DataManager.getInstance().addCharacter(new Character(Character.TYPE_SPACE));
        invalidate();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                touch_start(x, y);
                invalidate();
                break;
            case MotionEvent.ACTION_MOVE:
                touch_move(x, y);
                invalidate();
                break;
            case MotionEvent.ACTION_UP:
                touch_up(x, y);
                invalidate();
                break;
        }
        return true;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && DataManager.getInstance().getCharacters().size() > 0) {
            DataManager.getInstance().popCharacter();
            invalidate();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
}
