import os
app = defines['app']
files = [app]
symlinks = {'Applications': '/Applications'}
format = 'UDZO'
compression_level = 6
window_rect = ((200, 120), (546, 343))
default_view = 'icon-view'
show_icon_preview = False
show_toolbar = False; show_status_bar = False; show_pathbar = False; show_sidebar = False
icon_size = 96; text_size = 12
icon_locations = {os.path.basename(app): (150, 150), 'Applications': (400, 150)}
size = '9g'
