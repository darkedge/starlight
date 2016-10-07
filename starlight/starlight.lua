-- http://lua-users.org/wiki/ObjectOrientationTutorial

Vector = {}
Vector.__index = Vector
Vector.__add =
    function (lhs, rhs)
        return Vector.new(
            lhs.x + rhs.x,
            lhs.y + rhs.y,
            lhs.z + rhs.z)
    end
Vector.__sub =
    function (lhs, rhs)
        return Vector.new(
            lhs.x - rhs.x,
            lhs.y - rhs.y,
            lhs.z - rhs.z)
    end
Vector.__mul =
    function (lhs, rhs)
        assert(lhs ~= nil, "lhs was nil: " .. debug.traceback())
        assert(rhs ~= nil, "rhs was nil: " .. debug.traceback())
        assert(not (type(lhs) == "table" and type(rhs) == "table"), "error: mul got " .. type(lhs) .. " and " .. type(rhs) )
        if (type(lhs) == "number") then
            return Vector.new(
                lhs * rhs.x,
                lhs * rhs.y,
                lhs * rhs.z
            )
        elseif (type(rhs) == "number") then
            return Vector.new(
                lhs.x * rhs,
                lhs.y * rhs,
                lhs.z * rhs
            )
        end
    end


function Vector.new(x, y, z)
    -- Set Vector as metatable for empty object
    local self = setmetatable({}, Vector)
    self.x = x or 0
    self.y = y or 0
    self.z = z or 0
    return self
end

function normalize(v)
    local f = 1 / math.sqrt(lengthSqr(v))
    return Vector.new(
        v.x * f, v.y * f, v.z * f
    )
end

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

    -- Translation
    local SPEED = 75
    local translation = Vector.new()
    if KeyDown("MoveForward")	then translation = translation + C_Forward() end
    if KeyDown("StrafeLeft")	then translation = translation - C_Right() end
    if KeyDown("MoveBackward")	then translation = translation - C_Forward() end
    if KeyDown("StrafeRight")	then translation = translation + C_Right() end
    if KeyDown("Crouch")		then translation = translation - Vector.new(0, 1, 0) end
    if KeyDown("Jump")			then translation = translation + Vector.new(0, 1, 0) end
    if lengthSqr(translation) ~= 0 then
        local pos = GetPosition()
        pos = pos + normalize(translation) * SPEED * g_deltaTime
        SetPosition(pos)
    end
end
