icon_source_id = profile == '' ? 'org.gnome.Calendar' : 'org.gnome.Calendar.Devel'

foreach variant : ['scalable', 'symbolic']
  suffix = variant == 'symbolic' ? '-symbolic.svg' : '.svg'
  install_data(
    'hicolor' / variant / 'apps' / (icon_source_id + suffix),
    rename: application_id + suffix,
    install_dir: calendar_datadir / 'icons' / 'hicolor' / variant / 'apps',
  )
endforeach
