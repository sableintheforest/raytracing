#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <util/filesystem.h>
#include <util/shader.h>
#include <util/camera.h>
#include <util/model.h>
#include <util/assets.h>
#include <util/window.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void renderQuad();

// settings
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(-2.0f, 5.0f, 5.0f),glm::vec3(0,1,0), 0, -45);
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool leftButtonDown = false;
bool middleButtonDown = false;
bool rightButtonDown = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const char* APP_NAME = "Assignment 6";
int main()
{
    // controllable settings
    bool animateLight = false;
    int maxDepth = 3;

    // glfw: initialize and configure
    // ------------------------------
    InitWindowAndGUI(SCR_WIDTH, SCR_HEIGHT, APP_NAME);


    // OpenGL is initialized now, so we can use OpenGL functions (glFoo ...)
    // ------------------------------
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glDebugMessageCallback(MessageCallback, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    SetCursorPosCallback(mouse_callback);
    SetMouseButtonCallback(mouse_button_callback);
    SetScrollCallback(scroll_callback);
    SetFramebufferSizeCallback(framebuffer_size_callback);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    // set depth function to less than AND equal for skybox depth trick.
    glDepthFunc(GL_LEQUAL);
    // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // build and compile shaders
    // -------------------------
    Shader shader("../src/raytracing/raytracing.vs.glsl", "../src/raytracing/raytracing.fs.glsl");  
    shader.use();

    // lights
    // ------
    glm::vec3 lightPosition(-1.0f, 5.0f, 1.0f);
    


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // input
        // -----
        processInput(window);
        if (gui) {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                ImGui::Begin(APP_NAME);
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::SliderInt("ray depth", &maxDepth, 1, 10 );   // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::Checkbox("animate light", &animateLight);

                // a Button to reload the shader (so you don't need to recompile the cpp all the time)
                if (ImGui::Button("reload shaders")) {
                    shader.reload();
                    shader.use();
                }

                ImGui::End();
            }
        }


        // render
        // ------
        if (gui) ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        // render scene, supplying the convoluted irradiance map to the final shader.
        // ------------------------------------------------------------------------------------------
        glm::mat4 model = glm::mat4(1.0f);
        auto projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("view", view);
        shader.setVec3("camPos", camera.Position);
        shader.setInt("maxDepth", maxDepth);
        shader.setVec2("viewportSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
        

        // update the light sources
        //for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
        {
            glm::vec3 newPos = lightPosition;
            if(animateLight)  newPos = lightPosition + glm::vec3(sin(glfwGetTime() * 1.0) * 3.0, 0.0, 0.0);
            //newPos = lightPosition;
            shader.setVec3("lightPosition", newPos);
            //shader.setVec3("lightColor", lightColors[i]);
        }

        renderQuad();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        if (gui) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    if (width > 0 && height > 0)
    {
        glViewport(0, 0, width, height);
        SCR_WIDTH = width; SCR_HEIGHT = height;
    }
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (leftButtonDown)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever any mouse button is pressed, this callback is called
// ----------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)   leftButtonDown = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) middleButtonDown = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)  rightButtonDown = true;
    }
    else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)   leftButtonDown = false;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) middleButtonDown = false;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)  rightButtonDown = false;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}


// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
