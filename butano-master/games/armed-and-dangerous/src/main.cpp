/*
    License for Butano:

    Copyright (c) 2020-2022 Gustavo Valiente gustavo.valiente@protonmail.com
    zlib License, see LICENSE file.
*/

// Butano libraries
#include "bn_core.h" // Core libraries.
#include "bn_log.h"
#include "bn_sram.h"
#include "bn_music.h"
#include "bn_music_actions.h"
#include "bn_music_items.h"
#include "bn_sound_items.h"
#include "bn_sram.h"
#include "bn_math.h"
#include "bn_string.h"
#include "bn_keypad.h"
#include "bn_display.h"
#include "bn_random.h"
#include <bn_fixed_point.h>
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_palette_ptr.h"
#include "common_info.h"
#include "common_variable_8x8_sprite_font.h"
#include "bn_sprite_items_ball.h"
#include "bn_sprite_items_ball_new.h"
#include "bn_sprite_items_paddle_new.h"
#include "bn_regular_bg_items_bg.h"

int main()
{
    // Initialization
    // Make sure that all initialized Butano data types are placed
    // AFTER this line is called.
    bn::core::init();

    // Plays the initial music on startup.
    // ...
    // Make sure that the 'music item' matches the name of an .xm or .mod file in /audio!
    // The '1' represents your volume. Its data type is "bn::fixed".
    // This means that you can put 'bn::fixed(0.5)' instead of '1'
    // if you want it to play at half-volume.
    bn::music_items::amayadori.play(1);

    // Create a background at position 0,0.
    // We can move it around, but we don't want to for now.
    // ...
    // Notice that we're creating a "ptr" (pointer) object, which is
    // populated with the output of an "item" object's create function.
    // 'sprite_ptr' != 'sprite_item'
    bn::regular_bg_ptr bg = bn::regular_bg_items::bg.create_bg(0, 0);

    // This creates the two paddles.
    // The GBA resolution is 240 x 160, which means that, for a 64x64 paddle,
    // You want their X values to be at about -140 and 140, respectively.

    // We're flipping the right paddle, because the sprite is facing to the right.
    // It's important to save on sprites anywhere we can:
    // Don't forget: cartridges can only be around 16 MB in size!

    bn::random random;
    bn::vector<bn::fixed_point, 5> velocities;
    for (int i = 0; i < 5; ++i)
    {
        bn::fixed dx = bn::fixed::from_data(random.get_int(-1024, 1024)); // Random X velocity
        bn::fixed dy = bn::fixed::from_data(random.get_int(-1024, 1024)); // Random Y velocity
        velocities.push_back(bn::fixed_point(dx, dy));
    }
    // Let's put the ball in the center of the screen.
    bn::vector<bn::sprite_ptr, 5> sprites;
    bn::sprite_ptr t1 = bn::sprite_items::ball_new.create_sprite(0, 0);
    bn::sprite_ptr t2 = bn::sprite_items::ball_new.create_sprite(0, 10);
    bn::sprite_ptr t3 = bn::sprite_items::ball_new.create_sprite(10, 10);
    bn::sprite_ptr t4 = bn::sprite_items::ball_new.create_sprite(-10, 10);
    bn::sprite_ptr t5 = bn::sprite_items::ball_new.create_sprite(-10, -10);
    bn::sprite_ptr target = bn::sprite_items::ball_new.create_sprite(-20, -20);
    sprites.push_back(t1);
    sprites.push_back(t2);
    sprites.push_back(t3);
    sprites.push_back(t4);
    sprites.push_back(t5);

    // Here are some variables to get us started.
    int score = 0;               // We'll add a point when you score, and deduct a point when you fail.
    bool enemy_going_up = false; // This will help us remember which way the enemy paddle should move.

    // 'Delta' means change.
    // These two values represent, for each step of the gameloop,
    // which direction the ball should go. Negative means towards top-left, and positive is bottom-right.
    // For example,
    // delta_x = -1, delta_y = 1, would mean that the ball is moving towards the bottom-left side of the screen.
    // Increasing these numbers will make the ball move faster.
    int delta_x = 0;
    int delta_y = 0;

    // In Butano, as in many libraries, you need to initiatize an instance of a RANDOM object
    // in order to get random numbers. This is what we're doing here.

    // We'll want to set up a couple of Butano objects
    // in order to put text on the screen.
    bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);

    // A 'vector' is a data type that allows us to add or take away elements on a stack,
    // albeit a stack that can be accessed at any point. It's like an array, but it has
    // a built-in "size" value so that we can see how many elements are in it.
    // ..
    // We need to set up a vector of sprite_ptr to represent individual letters.
    // We can only have a max of 16 letters in this vector.
    bn::vector<bn::sprite_ptr, 16> text_sprites;

    // Let's go ahead and set up our default text!
    // The first two values represent X and Y.
    // The third is your constant text.
    // The fourth is your sprite_ptr vector.
    text_generator.generate(-6 * 16, -68, "(Press A to start)", text_sprites);

    // This is your main game loop.
    // Remember, the GBA is ONLY running your game -
    // You don't want it to be able to leave!
    // That's why the loop is set to never end.
    while (true)
    {
        if (score < 5) {
            for(int i = 0; i < 5; ++i)
            {
                // Update sprite positions based on velocities
                bn::fixed_point new_position = sprites[i].position() + velocities[i];

                // Screen boundaries
                if(new_position.x() < -120 || new_position.x() > 120)
                {
                    velocities[i].set_x(-velocities[i].x()); // Reverse X direction
                }
                if(new_position.y() < -80 || new_position.y() > 80)
                {
                    velocities[i].set_y(-velocities[i].y()); // Reverse Y direction
                }

                // Apply new position
                sprites[i].set_position(new_position);
            }
        }

        // If 'up' is being held, and we're not too far up,
        // Take our Y position and set it relative to where we currently are,
        // minus how far we want to move.
        if (bn::keypad::left_held() && target.x() > -120)
        {
            target.set_x(target.x() - 1);
        }

        // We use 'else' so that you can't hold both buttons down at
        // the same time, hypothetically. This seems silly on real
        // hardware, but you can do it in an emulator.
        // ...
        // Remember all potential use cases, to avoid bugs!
        else if (bn::keypad::right_held() && target.x() < 120)
        {
            target.set_x(target.x() + 1);
        }

        else if (bn::keypad::up_held() && target.y() > -70)
        {
            target.set_y(target.y() - 1);
        }

        else if (bn::keypad::down_held() && target.y() < 70)
        {
            target.set_y(target.y() + 1);
        }

        // When the BALL IS STILL, and the A BUTTON IS PRESSED,
        // We want to be able to set a new direction and speed for the ball.
        // That's what this function is checking for.
        if (bn::keypad::a_pressed() && delta_x == 0 && delta_y == 0)
        {

            // The 'generate' function fills up the selected vector,
            // so make sure to clear whatever is in it!
            text_sprites.clear();

            // We're setting up a string to represent the new value.
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);

            // In this function, I'm using a modulus.
            // They're a little confusing, but I'll do my best to explain here.
            // You can learn more about the modulus operator from here:
            // https://www.mathsisfun.com/definitions/modulo-operation.html
            // ...
            // In short, the Modulus allows you to "wrap around" a number.
            // Imagine that X represents a number counting up, and we were to
            // operate under a modulus of 3.
            // The output would look like this:
            /*

            x = 0       mod 3       output: 0
            x = 1       mod 3       output: 1
            x = 2       mod 3       output: 2
            x = 3       mod 3       output: 0
            x = 4       mod 3       output: 1
            x = 5       mod 3       output: 2

            */
            // It's pretty easy to wrap your head around once you get the hang of it!
            // Now... We're using it here, because the 'random.get_int()' function returns
            // a random number that could be up to 4,294,967,295! However, if we modulus
            // it to 4, then we will only get a random number between 0 and 3.
            // ...
            // I'm subtracting 2 because this means that, now, we have the potential to
            // have the ball move in either direction.
            // e.g.:
            /*

            og number = 0       -2      = -2
            og number = 1       -2      = -1
            og number = 2       -2      = 0
            og number = 4       -2      = 1
            og number = 5       -2      = 2

            */
            // Do you see how it allows us to have not only variable, speeds, but
            // potential random movement in both directions?
            // ...
            // Now, I'm including a while loop around this logic, because
            // there is always the potential chance that 'new_x' and 'new_y' will be 0,
            // and we don't want that or the ball will stand still or move in a flat line!

            // Let's play a sound once it's done deciding which direction to go.
            bn::sound_items::pong.play();
        }

        const bn::fixed collision_threshold = 5; // Adjust as needed

        if (bn::abs(t1.x() - target.x()) < collision_threshold && bn::abs(t1.y() - target.y()) < collision_threshold and bn::keypad::a_pressed())
        {
            BN_LOG(t1.y());
            score++;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t1.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }

// Repeat for t2, t3, t4, t5

        if (bn::abs(t2.x() - target.x()) < collision_threshold && bn::abs(t2.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t2.y());
            score++;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t2.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }

        if (bn::abs(t3.x() - target.x()) < collision_threshold && bn::abs(t3.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t3.y());
            score++;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t3.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }

        if (bn::abs(t4.x() - target.x()) < collision_threshold && bn::abs(t4.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t4.y());
            score++;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t4.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }

        if (bn::abs(t5.x() - target.x()) < collision_threshold && bn::abs(t5.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t1.y());
            score++;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t5.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }

        if (score == 5) {
            text_sprites.clear();
            bn::string<32> txt_score = "Level complete!";
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
        }

        // One last thing!
        // Let's talk about LOGGING.
        // We'll have the Butano logs add your score to the log when you click 'B'.
        // You can view these from within mGBA by selecting:
        /*

        1. Tools
        2. View logs

        */
        // This is ABSOLUTELY ESSENTIAL for debugging.

        if (bn::keypad::b_pressed())
        {
            BN_LOG(score);
        }

        // Do all the Butano things that we need to have done in the background.
        // If you don't call this, nothing will happen on the screen or through the speakers.
        bn::core::update();
    }
}

// And now we're done :)
// Good job!