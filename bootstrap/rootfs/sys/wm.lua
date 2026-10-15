local wm = {}

function wm:handle_messages()
	local messages = system.poll()

	for _, message in ipairs(messages) do
		if message.name == "WM_CREATE" then
			local data = message.payload
			self:window(message.sender, 5, 5, data.title, data.w, data.h)
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
end

function init()
	wm.windows = {}
end

function update()
	wm:handle_messages()
end

function draw()
	graphics.clear(0, 0, 255)

  for _, window in ipairs(wm.windows) do
    print("updating " .. window.pid .. " at <" .. window.x .. ", " .. window.y .. ">")
    print("size <" .. window.w .. ", " .. window.h .. ">")

    graphics.blit(window.pid, window.x, window.y)

    print("drawn.")
  end
end
