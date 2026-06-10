-- input_to_anim.lua
--
-- On-screen test driver for two Diorama features at once:
--   * 2D Input Actions  (DioramaInputRequestBus) -- reads a "move" Axis2D and a
--     "jump" Button mapped from WASD / arrows / left stick / space / gamepad A.
--   * 2D Animation State Machine (DioramaAnimStateMachineRequestBus) -- the "move"
--     magnitude is pushed into the graph's "speed" param and a jump press pulses its
--     "jump" trigger, so the graph switches idle <-> run (and a one-shot jump) itself.
--
-- To make the state switch unmistakable without art, the sprite is tinted by the
-- graph's CURRENT state (green = idle, white = run, orange = jump) and walked by the
-- move vector. Attach to a sprite entity that also has a 2D Input Actions component
-- (actions "move"/"jump") and a 2D Animation State Machine (params "speed"/"jump",
-- states idle/run/jump). The anim_input_demo.py builder sets all of that up.
local InputToAnim = {
    Properties = {
        Speed = { default = 6.0, description = "Move speed in world units per second" },
    },
}

function InputToAnim:OnActivate()
    self.speed = self.Properties.Speed or 6.0
    self.lastState = nil
    self.tickHandler = TickBus.Connect(self)
end

function InputToAnim:OnDeactivate()
    if self.tickHandler then self.tickHandler:Disconnect() end
end

function InputToAnim:OnTick(deltaTime, scriptTime)
    local id = self.entityId

    -- 1. Read the mapped actions (the 2D Input Actions component).
    local mx = DioramaInputRequestBus.Event.GetValue(id, "move")
    local my = DioramaInputRequestBus.Event.GetValueY(id, "move")
    local magnitude = math.sqrt(mx * mx + my * my)

    -- 2. Drive the state machine from that input.
    DioramaAnimStateMachineRequestBus.Event.SetFloat(id, "speed", magnitude)
    if DioramaInputRequestBus.Event.WasPressedThisFrame(id, "jump") then
        DioramaAnimStateMachineRequestBus.Event.SetTrigger(id, "jump")
    end

    -- 3. Move the sprite so the input is visible as motion.
    if magnitude > 0.01 then
        local p = TransformBus.Event.GetWorldTranslation(id)
        local step = self.speed * deltaTime
        TransformBus.Event.SetWorldTranslation(id, Vector3(p.x + mx * step, p.y + my * step, p.z))
    end

    -- 4. Tint by the graph's current state so the switch is unmistakable on screen.
    local state = DioramaAnimStateMachineRequestBus.Event.GetCurrentState(id)
    if state ~= self.lastState then
        self.lastState = state
        Debug.Log("Diorama anim state -> " .. tostring(state))
    end
    if state == "run" then
        DioramaSpriteRequestBus.Event.SetTint(id, 1.0, 1.0, 1.0, 1.0)
    elseif state == "jump" then
        DioramaSpriteRequestBus.Event.SetTint(id, 1.0, 0.6, 0.2, 1.0)
    else
        DioramaSpriteRequestBus.Event.SetTint(id, 0.4, 1.0, 0.4, 1.0)
    end
end

return InputToAnim
