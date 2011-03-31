<?php

/*
 * @author Chokri Tliche
 *
 */

/*
 * render module config Tabs
 * @param array $tabs a list of active tabs
 */

function theme_module_config_tabs($tabs) {
  if (empty($tabs)) {
    return NULL;
  }

  $output = '<div id="system-config-tabs"><ul class="menu">';
  $active_trail = arg(2);
  ksort($tabs);

  foreach ($tabs as $tab => $sorted_sections) {

    ksort($sorted_sections);
    $x = 0;

    foreach ($sorted_sections as $sections) {
      $tab_class = (!count($sections)) ? 'leaf ' : '';

      foreach ($sections as $module => $section) {
        if (++$x == 1) {
          $active_trail = ($active_trail == $module) ? 1 : 0;
          $output .= '<li class="' . $tab_class;
          $output .= ( $active_trail) ? 'expanded active-trail' : 'expanded';
          $output .= '">' . l($tab, 'admin/system-config') . '<ul>';
        }
        $output .= '<li class="leaf">';
        $output .= l($section['label'], 'admin/system-config/' . $module);
        $output .= '</li>';
      }
    }

    $output .= '</ul></li>';
  }
  $output .= '</div></ul>';

  return $output;
}