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
      glm::float64 pitch, yaw;
      
      glm::mat4 view, proj, cam;

      Camera(vk::Extent2D extent) {
        // Set projection matrix
        proj = glm::perspective(
            glm::radians(90.0),
            (glm::float64) (extent.width / extent.height),
            0.1,
            1000.0);
        
        // Set initial pos, pitch and yaw
        jump(glm::vec3(0.0, 0.0, -0.1), glm::half_pi<glm::float64>(), 0);
        build();
      }

      void move(glm::vec3 deltaPos, glm::float64 deltaPitch,glm::float64 deltaYaw)
      {
        pos += deltaPos;
        pitch += deltaPitch;
        yaw += deltaYaw;
      }

      void jump(glm::vec3 newPos, glm::float64 newPitch, glm::float64 newYaw)
      {
        pos = newPos;
        pitch = newPitch;
        yaw = newYaw;
      }

      // Credit to https://www.3dgep.com/understanding-the-view-matrix/
      // TODO:: Support changes to projection matrix (aspect resize mainly)
      void build()
      {
        glm::float64 cosPitch = glm::cos(pitch);
        glm::float64 sinPitch = glm::sin(pitch);
        glm::float64 cosYaw = glm::cos(yaw);
        glm::float64 sinYaw = glm::sin(yaw);
        glm::vec3 x = {cosYaw, 0, -sinYaw};
        glm::vec3 y = {sinYaw * sinPitch, cosPitch, cosYaw * sinPitch };
        glm::vec3 z = {sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw };

        view = glm::mat4(
            glm::vec4(x.x, x.y, z.y, 0),
            glm::vec4(x.y, y.y, z.y, 0),
            glm::vec4(x.z, y.z, z.z, 0),
            glm::vec4(-glm::dot(x, pos), -glm::dot(y, pos), -glm::dot(z, pos), 1));
        
        cam = proj * view;
      }
    };
}

#endif // NGFX_CAMERA_H
