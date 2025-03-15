package my.sdlc.testsdl;


import static org.libsdl.app.SDL.setContext;

import android.os.Bundle;

import org.libsdl.app.SDL;
import org.libsdl.app.SDLActivity;

public class Maze3DActivity extends SDLActivity {

     @Override
    protected String[] getLibraries() {
        return new String[]{ "maze3D"};
    }
}