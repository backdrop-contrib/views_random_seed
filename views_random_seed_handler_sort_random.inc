<?php
// $Id$

/**
 * Handle a random sort with seed.
 */
class views_random_seed_handler_sort_random extends views_handler_sort {

  // Options form.
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['order']['#access'] = FALSE;

    // User seed type.
    $form['user_seed_type'] = array(
      '#type' => 'radios',
      '#title' => t('User seed type'),
      '#options' => array(
        'same_per_user' => t('Use the same seed for every user'),
        'diff_per_user' => t('Use a different seed per user'),
      ),
      '#default_value' => isset($this->options['user_seed_type']) ? $this->options['user_seed_type'] : 'same_per_user',
    );

    // User seed type.
    $form['reset_seed_int'] = array(
      '#type' => 'radios',
      '#title' => t('Reset seed'),
      '#options' => array(
        'never' => t('Never'),
        'custom' => t('Custom'),
        '3600' => t('Every hour'),
        '28800' => t('Every day'),
      ),
      '#default_value' => isset($this->options['reset_seed_int']) ? $this->options['reset_seed_int'] : '3600',
    );
    $form['reset_seed_custom'] = array(
      '#type' => 'textfield',
      '#size' => 10,
      '#title' => t('Custom reset seed'),
      '#required' => TRUE,
      '#default_value' => isset($this->options['reset_seed_custom']) ? $this->options['reset_seed_custom'] : '300',
      '#description' => t('Define your own custom reset time, must be a number. Choose custom in the options above.'),
    );

    // Caching strategy.
    /*$caching = array(
      CACHE_DISABLED => t('Disabled'),
      CACHE_NORMAL => t('Normal (recommended for production sites, no side effects)'),
      CACHE_AGGRESSIVE => t('Aggressive (experts only, possible side effects)')
    );
    $form['anon_clear_cache'] = array(
      '#type' => 'radios',
      '#title' => t('Caching strategy when page cache is enabled'),
      '#options' => array(
        'no' => t('Let drupal page caching do its work for anonymous users'),
        'yes' => t('Do not cache pages for anonymous users'),
      ),
      '#description' => t('Current cache mode: @cache<br />Warning: choosing to not cache pages for anonymous users might have a big influence on performance for your site.', array('@cache' => $caching[variable_get('cache', CACHE_DISABLED)])),
      '#default_value' => isset($this->options['anon_clear_cache']) ? $this->options['anon_clear_cache'] : 'no',
    );*/
  }

  // Perform query.
  function query() {
    global $db_type;
    $seed = views_random_seed_calculate_seed($this->options, $this->view->name, $this->view->current_display);
    switch ($db_type) {
      case 'mysql':
      case 'mysqli':
        $formula = 'RAND('. $seed .')';
        break;
      case 'pgsql':
        $formula = 'RANDOM('. $seed .')';
        break;
    }
    $this->query->add_orderby(NULL, $formula, $this->options['order'], '_' . $this->field);
  }
}

/**
 * Calculate a seed.
 *
 * @param array $options The options for the random seed handler.
 * @param string $view_name The name of the view.
 * @param string $display The current display.
 *
 * @return int $seed Seed value which is a timestamp.
 */
function views_random_seed_calculate_seed($options, $view_name, $display) {
  $time = time();
  $seed_name = 'views_seed_name-'. $view_name .'-'. $display;
  $seed_time = 'views_seed_time-'. $view_name .'-'. $display;
  $seed = isset($_SESSION[$seed_name]) ? $_SESSION[$seed_name] : FALSE;

  // Create a first seed if necessary.
  if ($seed == FALSE) {
    $seed = _views_random_seed_generate_seed($options['user_seed_type'], $seed_name, $seed_time, $time);
  }

  // Reset seed or not ?
  if ($options['reset_seed_int'] != 'never') {
    $reset_time = $options['reset_seed_int'] == 'custom' ? $options['reset_seed_custom'] : $options['reset_seed_int'];
    if (($_SESSION[$seed_time] + $reset_time) < $time) {
      $seed = _views_random_seed_generate_seed($options['user_seed_type'], $seed_name, $seed_time, $time);
    }
  }

  // Return seed.
  return $seed;
}

/**
 * Helper function to generate a seed
 *
 * @param string $user_seed_type Type of user seed.
 * @param string $seed_name Name of the seed.
 * @param string $seed_time Time of the seed.
 * @param int $time Current timestamp.
 *
 * @return int seed value which is a timestamp.
 */
function _views_random_seed_generate_seed($user_seed_type, $seed_name, $seed_time, $time) {

  // Different per user, simply return $time.
  if ($user_seed_type == 'diff_per_user') {
    $seed = $time;
  }
  else {
    // Same for al users, get a stored variable.
    $seed = variable_get($seed_name, FALSE);
    if ($seed == FALSE) {
      $seed = variable_set($seed_name, $time);
    }
  }

  $_SESSION[$seed_time] = $time;
  $_SESSION[$seed_name] = $seed;
  return $seed;
}
