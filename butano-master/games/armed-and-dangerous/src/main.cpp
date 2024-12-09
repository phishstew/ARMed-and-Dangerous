/*
    License for Butano:

    Copyright (c) 2020-2022 Gustavo Valiente gustavo.valiente@protonmail.com
    zlib License, see LICENSE file.
*/

// Butano libraries
#include "bn_core.h"
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

    bn::core::init();
    bn::music_items::crystalclearloop.play(1); // start bg music
    bn::regular_bg_ptr bg = bn::regular_bg_items::bg.create_bg(0, 0); // create bg
    bn::random random;
    bn::vector<bn::fixed_point, 5> l1_velocities; // vector for easy targets' velocities
    bn::vector<bn::fixed_point, 5> l2_velocities; // vector for medium targets' velocities
    bn::vector<bn::fixed_point, 5> l3_velocities; // vector for hard targets' velocities
    bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
    bn::vector<bn::sprite_ptr, 64> text_sprites;
    bn::vector<int, 5> hit_count; // vector for number of hits of each target (for use in lv 3, since targets must be hit multiple times)
    for (int i = 0; i < 5; i++) {
        hit_count.push_back(0);
    }

    int score = 0; // initialize player score to 0
    int ammo_count = 7; // initialize player ammo ct to 7
    bool successful_shot = false; // variable to keep track of if a given shot is succesful or not
    bool all_shot = false; // variable to keep track of if all enemies have been shot
    bool l1_started = false; // variable to keep track of if the 1st level is in progress
    bool l2_initializer = false;
    bool l2_started = false; // variable to keep track of if the 2nd level is in progress
    bool l3_initializer = false;
    bool l3_started = false; // variable to keep track of if the 3rd level is in progress
    bool start_scrn = true;  // variable to keep track of if we are on the start screen; initialized to true since that's the first thing we need
    bool win_scrn = false; // variable to keep track of if we have reached the "game over" screen after player wins
    
    // text instructions (start screen)
    text_generator.generate(-6 * 16, -68, "Welcome (Press L to start/shoot)", text_sprites);
    text_generator.generate(-6 * 16, -55, "Press A to move left", text_sprites);
    text_generator.generate(-6 * 16, -40, "Press D to move right", text_sprites);
    text_generator.generate(-6 * 16, -25, "Press W to move up", text_sprites);
    text_generator.generate(-6 * 16, -10, "Press S to move down", text_sprites);
    
    // code for the start screen to check if A (on GBA) is pressed and start the game
    while(start_scrn){
        if (bn::keypad::a_pressed()) {
            // initialize variables for level 1
            l1_started = true;
            text_sprites.clear();
            start_scrn = false;
            score = 0;
            ammo_count = 7;
            all_shot = false;
            successful_shot = false;
            break;
        }
        bn::core::update();
    }

    // store velocities in their respective vectors
    for (int i = 0; i < 5; ++i)
    {
        bn::fixed dx = bn::fixed::from_data(random.get_int(-4096, 4096)); // Random X velocity
        bn::fixed dy = bn::fixed::from_data(random.get_int(-4096, 4096)); // Random Y velocity
        bn::fixed fast_dx = bn::fixed::from_data(random.get_int(-8192, 8192)); // Random X velocity
        bn::fixed fast_dy = bn::fixed::from_data(random.get_int(-8192, 8192)); // Random Y velocity
        l1_velocities.push_back(bn::fixed_point(dx, dy));
        l2_velocities.push_back(bn::fixed_point(fast_dx, fast_dy));
        l3_velocities.push_back(bn::fixed_point(fast_dx, fast_dy));
    }

    bn::vector<bn::sprite_ptr, 5> sprites; // vector to keep track of enemy sprites (level 1)
    // enemy sprite instantiation
    bn::sprite_ptr t1 = bn::sprite_items::ball.create_sprite(0, 0);
    bn::sprite_ptr t2 = bn::sprite_items::ball.create_sprite(0, 10);
    bn::sprite_ptr t3 = bn::sprite_items::ball.create_sprite(10, 10);
    bn::sprite_ptr t4 = bn::sprite_items::ball.create_sprite(-10, 10);
    bn::sprite_ptr t5 = bn::sprite_items::ball.create_sprite(-10, -10);
    bn::sprite_ptr target = bn::sprite_items::ball_new.create_sprite(-20, -20);
    // addition of enemy sprites to the vector
    sprites.push_back(t1);
    sprites.push_back(t2);
    sprites.push_back(t3);
    sprites.push_back(t4);
    sprites.push_back(t5);

    int delta_x = 0;
    int delta_y = 0;

    // code for level 1
    while (l1_started)
    {
        // this case represents the player failing lv 1
        if (ammo_count == 0 and score < 5)
        {
            // move all enemy sprites off screen
            t1.set_position(-500, 500);
            t2.set_position(-500, 500);
            t3.set_position(-500, 500);
            t4.set_position(-500, 500);
            t5.set_position(-500, 500);
            // leave the loop
            l1_started = false;
            all_shot = false;
            win_scrn = false;
            break;
        }

        // enemy sprite motion only takes place if the game is still in play (score < 5)
        if (score < 5) {
            // constantly update target positions
            for(int i = 0; i < 5; ++i)
            {
                bn::fixed_point new_position = sprites[i].position() + l1_velocities[i];

                if(new_position.x() < -120 || new_position.x() > 120)
                {
                    l1_velocities[i].set_x(-l1_velocities[i].x());
                }
                if(new_position.y() < -80 || new_position.y() > 80)
                {
                    l1_velocities[i].set_y(-l1_velocities[i].y());
                }

                sprites[i].set_position(new_position);
            }
        }

        // player controls for crosshair
        if (bn::keypad::left_held() && target.x() > -120)
        {
            target.set_x(target.x() - 1);
        }
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

        // start game if A pressed
        if (bn::keypad::a_pressed() && delta_x == 0 && delta_y == 0)
        {
            text_sprites.clear();

            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            bn::sound_items::pong.play();
        }

        const bn::fixed collision_threshold = 5; // collision detection threshold -- adjust as needed
        // collision detection for t1
        if (bn::abs(t1.x() - target.x()) < collision_threshold && bn::abs(t1.y() - target.y()) < collision_threshold and bn::keypad::a_pressed())
        {
            BN_LOG(t1.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t1.set_position(-500, 500);
            bn::sound_items::pong.play();
            for(int i = 0; i < 30; ++i)
            {
                bn::core::update();
            }

            // Play ping.wav sound after delay
            bn::sound_items::ping.play();
        }
        // collision detection for t2
        if (bn::abs(t2.x() - target.x()) < collision_threshold && bn::abs(t2.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t2.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t2.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t3
        if (bn::abs(t3.x() - target.x()) < collision_threshold && bn::abs(t3.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t3.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t3.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t4
        if (bn::abs(t4.x() - target.x()) < collision_threshold && bn::abs(t4.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t4.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t4.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t5
        if (bn::abs(t5.x() - target.x()) < collision_threshold && bn::abs(t5.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t1.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t5.set_position(-500, 500);
            // Play sound effect
            bn::sound_items::pong.play();
        }
        // no collision occurred but a shot was fired
        else if (bn::keypad::a_pressed() and !successful_shot) {
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
        }
        // winning condition (level 1)
        if (score == 5 and ammo_count >= 0) {
            all_shot = true;
            win_scrn = true;
            t1.set_position(-500, 500);
            t2.set_position(-500, 500);
            t3.set_position(-500, 500);
            t4.set_position(-500, 500);
            t5.set_position(-500, 500);
            l1_started = false;
            l2_initializer = true; // move to level 2 initialization
            break;
        }
        // reset this var before checking for another collision
        successful_shot = false;
        bn::core::update();
    }

    // initialize all conditions for level 2 before proceeding to the level
    while (l2_initializer) {

        if (ammo_count < 3) {
            ammo_count = 5; // if the player has less than 3 ammo after lv 1, reset to 5 ammo for lv 2 (since that is the minimum necessary to complete the level)
        }
        else {
            ammo_count += 3; // if they have 3 or more ammo left, give the player some extra ammo for the next level as a reward for doing well on lv 1
        }
        
        // reset target positions
        t1.set_position(0, 0);
        t2.set_position(0, 10);
        t3.set_position(10, 10);
        t4.set_position(-10, 10);
        t5.set_position(-10, -10);
        all_shot = false;
        win_scrn = false;
        l2_started = true; // start lv 2
        l2_initializer = false;
    }

    // level 2 code
    while (l2_started) {
        
        if (ammo_count == 0 and score < 10) // total score never resets between levels
        {
            // move all enemy sprites off the screen
            t1.set_position(-500, 500);
            t2.set_position(-500, 500);
            t3.set_position(-500, 500);
            t4.set_position(-500, 500);
            t5.set_position(-500, 500);
            // leave the loop
            l2_started = false;
            all_shot = false;
            win_scrn = false;
            break;
        }
        
        if (score < 10) {
            // constantly update target positions
            for(int i = 0; i < 5; ++i)
            {
                bn::fixed_point new_position = sprites[i].position() + l2_velocities[i];

                if(new_position.x() < -120 || new_position.x() > 120)
                {
                    l2_velocities[i].set_x(-l2_velocities[i].x());
                }
                if(new_position.y() < -80 || new_position.y() > 80)
                {
                    l2_velocities[i].set_y(-l2_velocities[i].y());
                }

                sprites[i].set_position(new_position);
            }
        }
        
        // controls, again
        if (bn::keypad::left_held() && target.x() > -120)
        {
            target.set_x(target.x() - 1);
        }
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

        // start game if A pressed
        if (bn::keypad::a_pressed() && delta_x == 0 && delta_y == 0)
        {
            text_sprites.clear();

            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            bn::sound_items::pong.play();
        }

        const bn::fixed collision_threshold = 5; // collision detection threshold -- adjust as needed
        // collision detection for t1
        if (bn::abs(t1.x() - target.x()) < collision_threshold && bn::abs(t1.y() - target.y()) < collision_threshold and bn::keypad::a_pressed())
        {
            BN_LOG(t1.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t1.set_position(-500, 500);
            bn::sound_items::pong.play();
            for(int i = 0; i < 30; ++i)
            {
                bn::core::update();
            }

            // Play ping.wav sound after delay
            bn::sound_items::ping.play();
        }
        // collision detection for t2
        if (bn::abs(t2.x() - target.x()) < collision_threshold && bn::abs(t2.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t2.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t2.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t3
        if (bn::abs(t3.x() - target.x()) < collision_threshold && bn::abs(t3.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t3.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t3.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t4
        if (bn::abs(t4.x() - target.x()) < collision_threshold && bn::abs(t4.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t4.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t4.set_position(-500, 500);

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t5
        if (bn::abs(t5.x() - target.x()) < collision_threshold && bn::abs(t5.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t1.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            t5.set_position(-500, 500);
            // Play sound effect
            bn::sound_items::pong.play();
        }
        // no collision occurred but a shot was fired
        else if (bn::keypad::a_pressed() and !successful_shot) {
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
        }
        // winning condition (level 1)
        if (score == 10 and ammo_count >= 0) {
            all_shot = true;
            win_scrn = true;
            t1.set_position(-500, 500);
            t2.set_position(-500, 500);
            t3.set_position(-500, 500);
            t4.set_position(-500, 500);
            t5.set_position(-500, 500);
            l2_started = false;
            l3_initializer = true; // move to level 3 initialization
            break;
        }
        // reset this var before checking for another collision
        successful_shot = false;
        bn::core::update();

    }

    // level 3 initialization
    while (l3_initializer) {
        if (ammo_count < 3) {
            ammo_count = 12; // if the player has less than 3 ammo after lv 1, reset to 5 ammo for lv 2 (since that is the minimum necessary to complete the level)
        }
        else {
            ammo_count += 12; // if they have 3 or more ammo left, give the player some extra ammo for the next level as a reward for doing well on lv 1
        }
        // reset target positions
        t1.set_position(0, 0);
        t2.set_position(0, 10);
        t3.set_position(10, 10);
        t4.set_position(-10, 10);
        t5.set_position(-10, -10);
        all_shot = false;
        win_scrn = false;
        l3_started = true; // start lv 2
        l3_initializer = false;
    }

    // level 3 code
    while (l3_started) {
        if (ammo_count == 0 and score < 15) // total score never resets between levels
        {
            // move all enemy sprites off the screen
            t1.set_position(-500, 500);
            t2.set_position(-500, 500);
            t3.set_position(-500, 500);
            t4.set_position(-500, 500);
            t5.set_position(-500, 500);
            // leave the loop
            l3_started = false;
            all_shot = false;
            win_scrn = false;
            break;
        }
        
        if (score < 15) {
            // constantly update target positions
            for(int i = 0; i < 5; ++i)
            {
                bn::fixed_point new_position = sprites[i].position() + l3_velocities[i];

                if(new_position.x() < -120 || new_position.x() > 120)
                {
                    l3_velocities[i].set_x(-l3_velocities[i].x());
                }
                if(new_position.y() < -80 || new_position.y() > 80)
                {
                    l3_velocities[i].set_y(-l3_velocities[i].y());
                }

                sprites[i].set_position(new_position);
            }
        }
        
        // faster controls as a buffed power for the player
        if (bn::keypad::left_held() && target.x() > -120)
        {
            target.set_x(target.x() - 5);
        }
        else if (bn::keypad::right_held() && target.x() < 120)
        {
            target.set_x(target.x() + 5);
        }

        else if (bn::keypad::up_held() && target.y() > -70)
        {
            target.set_y(target.y() - 5);
        }

        else if (bn::keypad::down_held() && target.y() < 70)
        {
            target.set_y(target.y() + 5);
        }

        // start game if A pressed
        if (bn::keypad::a_pressed() && delta_x == 0 && delta_y == 0)
        {
            text_sprites.clear();

            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            bn::sound_items::pong.play();
        }

        const bn::fixed collision_threshold = 5; // collision detection threshold -- adjust as needed
        // collision detection for t1
        if (bn::abs(t1.x() - target.x()) < collision_threshold && bn::abs(t1.y() - target.y()) < collision_threshold and bn::keypad::a_pressed())
        {
            BN_LOG(t1.y());
            hit_count[0]++;
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            if (hit_count[0] == 2) {
                t1.set_position(-500, 500);
            }
            bn::sound_items::pong.play();
            for(int i = 0; i < 30; ++i)
            {
                bn::core::update();
            }
            // Play ping.wav sound after delay
            bn::sound_items::ping.play();
        }
        // collision detection for t2
        if (bn::abs(t2.x() - target.x()) < collision_threshold && bn::abs(t2.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t2.y());
            successful_shot = true;
            score++;
            hit_count[1]++;
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            if (hit_count[1] == 2) {
                t2.set_position(-500, 500);
            }

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t3
        if (bn::abs(t3.x() - target.x()) < collision_threshold && bn::abs(t3.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t3.y());
            successful_shot = true;
            score++;
            ammo_count--;
            hit_count[2]++;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            if (hit_count[2] == 2) {
                t3.set_position(-500, 500);
            }

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t4
        if (bn::abs(t4.x() - target.x()) < collision_threshold && bn::abs(t4.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t4.y());
            successful_shot = true;
            score++;
            ammo_count--;
            hit_count[3]++;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            if (hit_count[3] == 2) {
                t4.set_position(-500, 500);
            }

            // Play sound effect
            bn::sound_items::pong.play();
        }
        // collision detection for t5
        if (bn::abs(t5.x() - target.x()) < collision_threshold && bn::abs(t5.y() - target.y()) < collision_threshold  and bn::keypad::a_pressed())
        {
            BN_LOG(t1.y());
            successful_shot = true;
            score++;
            ammo_count--;
            text_sprites.clear();
            hit_count[4]++;
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
            delta_y = -delta_y;
            if (hit_count[4] == 2) {
                t5.set_position(-500, 500);
            }
            // Play sound effect
            bn::sound_items::pong.play();
        }
        // no collision occurred but a shot was fired
        else if (bn::keypad::a_pressed() and !successful_shot) {
            ammo_count--;
            text_sprites.clear();
            bn::string<32> txt_score = "Score: " + bn::to_string<32>(score) + " Ammo: " + bn::to_string<32>(ammo_count);
            text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
        }
        // winning condition (level 1)
        if (score == 15 and ammo_count >= 0) {
            all_shot = true;
            win_scrn = true;
            t1.set_position(-500, 500);
            t2.set_position(-500, 500);
            t3.set_position(-500, 500);
            t4.set_position(-500, 500);
            t5.set_position(-500, 500);
            l2_started = false;
            l3_initializer = true; // move to level 3 initialization
            break;
        }
        // reset this var before checking for another collision
        successful_shot = false;
        bn::core::update();
    }

    // reset this var after a level is exited
    successful_shot = false;
    // player ran out of ammo but not all enemies were shot (lose screen)
    while (!all_shot) {
        text_sprites.clear();
        bn::string<32> txt_score = "You lost! Press L for main menu";
        text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
        if (bn::keypad::a_pressed()) {
            start_scrn = true;
            break;
        }
        bn::core::update();
    }

    // player shot all enemies (win screen)
    while (all_shot) {
        text_sprites.clear();
        bn::string<32> txt_score = "You won! Press A for main menu";
        text_generator.generate(-6 * 16, -68, txt_score, text_sprites);
        if (bn::keypad::left_pressed()) {
            start_scrn = true;
            all_shot = false;
            break;
        }
        bn::core::update();
    }
}