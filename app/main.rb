$gtk.reset

$gtk.ffi_misc.gtk_dlopen("ext")
include FFI::CExt

def tick args
  args.outputs.background_color = [204, 204, 204]
  prepare(args) if args.state.tick_count == 0
  handle_input(args)
  update(args)
end

def prepare(args)
  # put the source image in the background (could fix these hard coded values)
  args.outputs.static_sprites << [(args.grid.w - 512) / 2, (args.grid.h - 512) / 2, 512, 512, '/sprites/lena.jpg']
  # tell C to load the pixel arrays
  load_image_from_ext(args)
  # setup names for the effects
  args.state.modes = %w"normal red-channel green-channel blue-channel desaturate threshold posterize"
  # set the mode to "normal"
  args.state.mode_num = 0
  # location of background image (could fix hard coded values)
  args.state.im_x = (args.grid.w - 512) / 2 
  args.state.im_y = (args.grid.h - 512) / 2
end

# updates magnifying glass image every tick
def update(args)
  m_x = args.inputs.mouse.x
  m_y = args.inputs.mouse.y
  # location of mouse relative to location of background image
  # nasty hardcoded values to be fixed
  m_x_off = m_x - args.state.im_x - 50
  m_y_off = 516 - m_y + 50 
  
  # tell C to update the image (updates the :lena render target)
  # casting ints to ints just to be sure
  ext_update_image(m_x_off.to_i, m_y_off.to_i, args.state.mode_num.to_i)
  # render the magnifying glass
  args.outputs.primitives << [m_x - 100, m_y - 100, 200, 200, :lena].sprite
  # draw a circle around the magnifying glass or... well it's not pretty
  args.outputs.primitives << [m_x - 110, m_y - 110, 220, 220, 'sprites/round.png'].sprite
  # display the name of the current effect
  args.outputs.labels << [385, 650, "mode: #{args.state.modes[args.state.mode_num]}"]
end

# called at the start to tell C to load images
def load_image_from_ext(args)
  # pointers which C will fill with image dimensions
  # (these can be used to eliminate the ugly hard coded values above)
  im_w = IntPointer.new
  im_h = IntPointer.new
  al_w = IntPointer.new
  al_h = IntPointer.new
  # make it so
  ext_load_image("sprites/lena.jpg", im_w, im_h, "sprites/alpha.png", al_w, al_h)
  # remember the dimensions
  args.state.w = al_w.value
  args.state.h = al_h.value
end

# click to change the current filter
def handle_input(args)
  args.state.mode_num += 1 if args.inputs.mouse.click
  args.state.mode_num = 0 if args.state.mode_num == args.state.modes.length
end
