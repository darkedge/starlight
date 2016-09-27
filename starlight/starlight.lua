function lengthSqr(vec)
	return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z
end

function MoveCamera()
    -- TODO: probably need to check if i'm typing something in ImGui or not

    --[[
    if (KeyPressed("MouseGrabToggle")) then
        SetMouseGrabbed(not IsMouseGrabbed())
    end

    -- Reset
    if KeyPressed("Reset") then
        ResetPosition(gameInfo)
        currentRotation = { x=0, y=0 }
        lastRotation = { x=0, y=0 }
    end

    -- Rotation
    --local ROT_SPEED = 0.0025
    if KeyDown("LookLeft") then currentRotation.y = currentRotation.y + 2.0 * s_deltaTime end
    if KeyDown("LookRight") then currentRotation.y = currentRotation.y - 2.0 * s_deltaTime end
    if KeyDown("LookDown") then currentRotation.x = currentRotation.x + 2.0 * s_deltaTime end
    if KeyDown("LookUp") then currentRotation.x = currentRotation.x - 2.0 * s_deltaTime end
    --currentRotation -= ROT_SPEED * input::GetMouseDelta()
    if currentRotation.x < -89.0 * DEG2RAD then
        currentRotation.x = -89.0 * DEG2RAD
    end
    if currentRotation.x > 89.0 * DEG2RAD then
        currentRotation.x = 89.0 * DEG2RAD
    end
    if currentRotation.x ~= lastRotation.x or currentRotation.y ~= lastRotation.y then
        player.SetRotation(Quat.rotationY(currentRotation.y) * Quat.rotationX(currentRotation.x))
        lastRotation = currentRotation
    end]]

    --[[ Translation
    local SPEED = 300.0
    local translation = {x=0, y=0, z=0}
    if KeyDown("MoveForward")	then translation = translation + C_Forward() end
    if KeyDown("StrafeLeft")	then translation = translation - C_Right() end
    if KeyDown("MoveBackward")	then translation = translation - C_Forward() end
    if KeyDown("StrafeRight")	then translation = translation + C_Right() end
    if KeyDown("Crouch")		then translation = translation - {x=0, y=1, z=0} end
    if KeyDown("Jump")			then translation = translation + {x=0, y=1, z=0} end
    if lengthSqr(translation) ~= 0.0 then
        local pos = GetPosition()
        pos = pos + normalize(translation) * SPEED * s_deltaTime
        SetPosition(pos)
    end]]
    KeyDown("MoveForward")
end
