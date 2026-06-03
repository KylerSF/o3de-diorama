-- player_move_2d.lua
--
-- A reusable top-down/side 2D player controller: moves the entity in the XY plane
-- from the "move_x" / "move_y" input events (WASD + arrows + left stick, see
-- diorama/input/diorama_move.inputbindings). Attach it to a sprite entity that also
-- has an Input component pointing at those bindings. This is the movement half of
-- the 2.5D quick-start (Docs/howto/17-quickstart.md); pair it with a Diorama Camera
-- (or make_active_camera) and a ground sprite for a playable scene.
local PlayerMove2D = {
    Properties = {
        Speed = { default = 8.0, description = "Move speed in world units per second" },
    },
}

function PlayerMove2D:OnActivate()
    self.speed = self.Properties.Speed or 8.0
    self.moveX = 0.0
    self.moveY = 0.0

    self.idMoveX = InputEventNotificationId("move_x")
    self.idMoveY = InputEventNotificationId("move_y")
    self.hMoveX = InputEventNotificationBus.Connect(self, self.idMoveX)
    self.hMoveY = InputEventNotificationBus.Connect(self, self.idMoveY)

    self.tickHandler = TickBus.Connect(self)
end

function PlayerMove2D:OnDeactivate()
    if self.tickHandler then self.tickHandler:Disconnect() end
    if self.hMoveX then self.hMoveX:Disconnect() end
    if self.hMoveY then self.hMoveY:Disconnect() end
end

-- Pressed (key down) and Held (analog / key repeat) both set the axis; Released
-- clears it, so movement tracks the live key/stick state.
function PlayerMove2D:Set(value)
    local id = InputEventNotificationBus.GetCurrentBusId()
    if id == self.idMoveX then
        self.moveX = value
    elseif id == self.idMoveY then
        self.moveY = value
    end
end

function PlayerMove2D:OnPressed(value)
    self:Set(value)
end

function PlayerMove2D:OnHeld(value)
    self:Set(value)
end

function PlayerMove2D:OnReleased(value)
    self:Set(0.0)
end

function PlayerMove2D:OnTick(deltaTime, scriptTime)
    if self.moveX == 0.0 and self.moveY == 0.0 then
        return
    end
    local p = TransformBus.Event.GetWorldTranslation(self.entityId)
    local step = self.speed * deltaTime
    TransformBus.Event.SetWorldTranslation(
        self.entityId, Vector3(p.x + self.moveX * step, p.y + self.moveY * step, p.z))
end

return PlayerMove2D
