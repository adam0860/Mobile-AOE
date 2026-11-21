package org.dawnofkings.game;

import org.libsdl.app.SDLActivity;

/** Thin launcher: all game code is native C++ (see ../../../../../../src). */
public class GameActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "SDL2_ttf", "main" };
    }
}
