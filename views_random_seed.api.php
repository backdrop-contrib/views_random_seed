<?php
/**
 * Generate a custom seed value for consistent display of random views.
 *
 * Views Random Seed module generates a seed at set intervals based on the view
 * configuration. This seed value defaults to the timestamp at which the seed
 * was generated. This hook can be used to create a custom seed based on
 * conditions other than time-based seeds.
 */
function hook_views_random_seed_calculate($seed_name, $view_name, $display_name, $options) {
  // Use the same seed value for all requests that originate from Lighthouse
  // visual regression testing.
  if (strpos($_REQUEST['User-Agent'], 'Chrome-Lighthouse')) {
    return 0;
  }
}
