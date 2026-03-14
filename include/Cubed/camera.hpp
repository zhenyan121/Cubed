#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Player;


class Camera {
private:

    bool m_firse_mouse = true;
    Player* m_player;
    float m_last_mouse_x, m_last_mouse_y;
    glm::vec3 m_camera_pos;
    

public:

    Camera();

    void update_move_camera();

    void camera_init(Player* player);

    void update_cursor_position_camera(double xpos, double ypos);

    const glm::mat4 get_camera_lookat() const;
    const glm::vec3& get_camera_pos() const;
};


