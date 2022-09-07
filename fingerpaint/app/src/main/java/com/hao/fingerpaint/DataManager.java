package com.hao.fingerpaint;

import android.graphics.Bitmap;

import java.text.BreakIterator;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by test on 16-3-9.
 */
public class DataManager {

    /// singleton
    static DataManager dataManager;
    public static DataManager getInstance() {
        if (dataManager == null) {
            dataManager = new DataManager();
        }
        return dataManager;
    }

    ///
    /// characters
    ///

    private ArrayList<Character> mCharacters = new ArrayList<Character>();

    ArrayList<Character> getCharacters() {
        return mCharacters;
    }

    public void addCharacter(Character c) {
        mCharacters.add(c);
    }

    public Character popCharacter() {
        Character c = null;
        int size = mCharacters.size();
        if (size > 0) {
            c = mCharacters.remove(size-1);
        }
        return  c;
    }


    ///
    /// picture
    ///

    private Bitmap mPicture;

    void setmPicture(Bitmap picture) {
        mPicture = picture;
    }

    Bitmap getmPicture() {
        return mPicture;
    }

}
