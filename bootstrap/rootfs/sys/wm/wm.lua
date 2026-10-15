local wm = {}

function wm:handle_messages()
	local messages = system.poll()

	for _, message in ipairs(messages) do
		if message.name == "WM_CREATE" then
			local data = message.payload
			self:window(message.sender, self.nx, self.ny, data.title, data.w, data.h)
		end
	end
end

function wm:window(pid, x, y, title, w, h)
	table.insert(self.windows, {
		title = title,
		x = x, y = y,
		w = w, h = h,

		pid = pid
	})

  self.nx = self.nx + self.no
  self.ny = self.ny + self.no
end

function init()
  system.register("display")
  graphics.window(420, 270, "StraWM")

  wm.windows = {}

  wm.nx = 20
  wm.ny = 20

  wm.no = 20
end

function update()
	wm:handle_messages()
end

function draw()
	graphics.clear(0, 0, 255)

  for _, window in ipairs(wm.windows) do
    graphics.blit(window.pid, window.x, window.y)
  end
end
