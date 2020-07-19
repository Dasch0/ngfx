#ifndef NGFX_CAMERA_H
#define NGFX_CAMERA_H

#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace ngfx
{
  class Camera {
    public:
      glm::vec3 pos;
      glm::float64 pitch, yaw, roll;
      
      glm::mat4 view, proj, cam;

      Camera(vk::Extent2D extent) {
        // Set projection matrix
        proj = glm::perspective(
            glm::radians(90.0),
            (glm::float64) (extent.width / extent.height),
            0.1,
            1000.0);
        
        // Set initial pos, pitch and yaw
        jump(glm::vec3(0.0, 0.0, .1), 0, 0, 0);
        build();
      }

      void move(
          glm::vec3 deltaPos,
          glm::float64 deltaPitch,
          glm::float64 deltaYaw,
          glm::float64 deltaRoll)
      {
        pos += deltaPos;
        pitch += deltaPitch;
        yaw += deltaYaw;
        roll += deltaRoll;
      }

      void jump(
          glm::vec3 newPos,
          glm::float64 newPitch,
          glm::float64 newYaw,
          glm::float64 newRoll)
      {
        pos = newPos;
        pitch = newPitch;
        yaw = newYaw;
        roll = newRoll;
      }

      // Credit to https://www.3dgep.com/understanding-the-view-matrix/
      // TODO:: Support changes to projection matrix (aspect resize mainly)
      void build()
      {
        pitch = glm::min(pitch, glm::radians(90.0));
        pitch = glm::max(pitch, glm::radians(-90.0));

        if (yaw < glm::radians(0.0))
        {
          yaw += glm::radians(360.0);
        }
        if (yaw > glm::radians(360.0))
        {
          yaw -= glm::radians(360.0);
        }

        if (roll < glm::radians(0.0)) {
          roll += glm::radians(360.0);
        } if (roll > glm::radians(360.0)) {
          roll -= glm::radians(360.0);
        }

        // Pitch = rotation about x axis
        // yaw = rotation about y axis
        // roll = rotation about z axis

        glm::float64 cosPitch = glm::cos(pitch);
        glm::float64 sinPitch = glm::sin(pitch);
        glm::float64 cosYaw = glm::cos(yaw);
        glm::float64 sinYaw = glm::sin(yaw);
        glm::float64 cosRoll = glm::cos(roll);
        glm::float64 sinRoll = glm::sin(roll);
        
        // Compute 3 axis rotation matrix (RxRyRz)
        glm::vec3 x = {
          cosYaw * cosRoll,
          sinPitch * sinYaw * cosRoll + cosPitch * sinRoll, 
          -(cosPitch * sinYaw * cosRoll) + sinPitch * sinRoll
        };
        glm::vec3 y = {
          -cosYaw * sinRoll,
          -(sinPitch * sinYaw * sinRoll) + cosPitch * cosRoll,
          cosPitch * sinYaw * sinRoll + sinPitch * cosRoll
        };
        glm::vec3 z = {
          sinYaw,
          -(sinPitch * cosYaw),
          cosPitch * cosYaw
        };

        // Invert via transpose
        view = glm::mat4(
            glm::vec4(x.x, y.x, z.x, 0),
            glm::vec4(x.y, y.y, z.y, 0),
            glm::vec4(x.z, y.z, z.z, 0),
            glm::vec4(-glm::dot(x, pos), -glm::dot(y, pos), -glm::dot(z, pos), 1));
        
        cam = proj * view;
      }
    };
}

#endif // NGFX_CAMERA_H1
