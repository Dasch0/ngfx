#ifndef NGFX_CAMERA_H
#define NGFX_CAMERA_H

#include "glm/fwd.hpp"
#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "fast_buffer.hpp"

namespace ngfx
{
  template<size_t N>
  class Camera {
    public:
      glm::vec3 pos[N];
      glm::float64 pitch[N];
      glm::float64 yaw[N];
      glm::float64 roll[N];
      
      glm::mat4 view[N];
      glm::mat4 proj[N];
      FastBuffer<glm::mat4, N> cam;

      Camera(vk::Extent2D extent) {
        // Set projection matrix
        
        for (size_t i = 0; i < N; i++) {
          proj = glm::perspective(
              glm::radians(90.0),
              (glm::float64) (extent.width / extent.height),
              0.1,
              1000.0);
         
          // Set initial pos, pitch and yaw
          jump(glm::vec3(0.0, 0.0, .1), 0, 0, 0);
        }
        
        build();
      }

      void move(
          glm::vec3 *deltaPos,
          glm::float64 *deltaPitch,
          glm::float64 *deltaYaw,
          glm::float64 *deltaRoll
          )
      {
        for(size_t i = 0; i < N; i++) {  
          pos[i] += deltaPos[i];
          pitch[i] += deltaPitch[i];
          yaw[i] += deltaYaw[i];
          roll[i] += deltaRoll[i];
        }
      }
      
      void jump(
          glm::vec3 *newPos,
          glm::float64 *newPitch,
          glm::float64 *newYaw,
          glm::float64 *newRoll)
      {
        for(size_t i = 0; i < N; i++)
        {
          pos[i] = newPos[i];
          pitch[i] = newPitch[i];
          yaw[i] = newYaw[i];
          roll[i] = newRoll[i];
        }
      }

      // Credit to https://www.3dgep.com/understanding-the-view-matrix/
      // TODO:: Support changes to projection matrix (aspect resize mainly)
      void build()
      {
        for (size_t i = 0; i < N; i++) {
          pitch[i] = glm::min(pitch[i], glm::radians(90.0));
          pitch[i] = glm::max(pitch[i], glm::radians(-90.0));

          if (yaw[i] < glm::radians(0.0))
          {
            yaw[i] += glm::radians(360.0);
          }
          if (yaw[i] > glm::radians(360.0))
          {
            yaw[i] -= glm::radians(360.0);
          }

          if (roll[i] < glm::radians(0.0)) {
            roll[i] += glm::radians(360.0);
          } if (roll[i] > glm::radians(360.0)) {
            roll[i] -= glm::radians(360.0);
          }

          // Pitch = rotation about x axis
          // yaw = rotation about y axis
          // roll = rotation about z axis

          glm::float64 cosPitch = glm::cos(pitch[i]);
          glm::float64 sinPitch = glm::sin(pitch[i]);
          glm::float64 cosYaw = glm::cos(yaw[i]);
          glm::float64 sinYaw = glm::sin(yaw[i]);
          glm::float64 cosRoll = glm::cos(roll[i]);
          glm::float64 sinRoll = glm::sin(roll[i]);
          
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
          view[i] = glm::mat4(
              glm::vec4(x.x, y.x, z.x, 0),
              glm::vec4(x.y, y.y, z.y, 0),
              glm::vec4(x.z, y.z, z.z, 0),
              glm::vec4(
                -glm::dot(x, pos[i]),
                -glm::dot(y, pos[i]),
                -glm::dot(z, pos[i]),
                1));
          
          cam.data[i] = proj[i] * view[i];
      }
    };
}

#endif // NGFX_CAMERA_H1
