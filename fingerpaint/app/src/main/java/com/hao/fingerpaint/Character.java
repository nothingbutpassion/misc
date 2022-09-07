package com.hao.fingerpaint;

import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;

import java.util.ArrayList;


public class Character {

    public static final int TYPE_PATH = 0x01;
    public static final int TYPE_SPACE = 0x02;
    public static final int TYPE_ENTER = 0x04;

    private int type;

    private RectF bound = new RectF();
    private Paint paint;
    private ArrayList<Path> paths = new ArrayList<Path>();

    public Character(int type) {
        this.type = type;
    }

    public Character() {
        this(TYPE_PATH);
    }


    public int getType() {
        return type;
    }

    public ArrayList<Path> getPaths() {
        return paths;
    }

    public void addPath(Path path) {
        paths.add(path);
    }

    public RectF getOriginalBound() {
        RectF rect = new RectF();
        for (Path path: paths) {
            RectF r = new RectF();
            path.computeBounds(r, true);
            rect.left = Math.min(r.left, rect.left);
            rect.top = Math.min(r.top, rect.top);
            rect.right = Math.max(r.right, rect.right);
            rect.bottom = Math.max(r.bottom, rect.bottom);
        }
        return  rect;
    }

    public RectF getBound() {
        return bound;
    }

    public void setBound(RectF rect) {
        bound.set(rect);
    }

    public Paint getPaint() {
        return paint;
    }

    public void setPaint(Paint paint) {
        this.paint = paint;
    }
}

