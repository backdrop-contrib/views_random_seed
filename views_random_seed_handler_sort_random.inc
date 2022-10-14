<?php

/**
 * @file
 * Views random seed order class.
 */

/**
 * Handle a random sort with seed.
 */
class views_random_seed_handler_sort_random extends views_handler_sort {

  function can_expose() {
    return FALSE;
  }

  function option_definition() {
    $options = parent::option_definition();

    $options['user_seed_type'] = array('default' => 'same_per_user');
    $options['reset_seed_int'] = array('default' => '3600');
    $options['reset_seed_custom'] = array('default' => '300');
    $options['shared_seed'] = array('default' => '0');
    $options['custom_seed'] = array('default' => '');

    return $options;
  }

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
      '#title' => t('Reset seed interval'),
      '#options' => array(
        'never' => t('Never'),
        '3600' => t('Every hour'),
        '28800' => t('Every day'),
        'custom' => t('Custom'),
      ),
      '#default_value' => isset($this->options['reset_seed_int']) ? $this->options['reset_seed_int'] : '3600',
    );
    $form['reset_seed_custom'] = array(
      '#type' => 'number',
      '#size' => 10,
      '#step' => 1,
      '#title' => t('Custom reset length'),
      '#default_value' => isset($this->options['reset_seed_custom']) ? $this->options['reset_seed_custom'] : '300',
      '#description' => t('Define your own custom reset time, must be a number and is in seconds. Choose custom in the options above.'),
      '#states' => array(
        'visible' => array(
          '[name="options[reset_seed_int]"]' => array('value' => 'custom'),
        ),
      ),
    );
    $form['shared_seed'] = array(
      '#type' => 'radios',
      '#title' => t('Per-display seeds'),
      '#options' => array(
        '0' => t('Each display uses a separate seed'),
        '1' => t('All displays in this view share the same seed'),
      ),
      '#default_value' => isset($this->options['shared_seed']) ? $this->options['shared_seed'] : '0',
      '#description' => t('Share the same seed across displays if needing to maintain the same random order between displays and prevent duplicates.'),
    );
  }

  // Perform query.
  function query() {
    $db_type = db_driver();
    $seed = views_random_seed_calculate_seed($this->view->name, $this->view->current_display, $this->options);
    switch ($db_type) {
      case 'mysql':
      case 'mysqli':
        $formula = 'RAND(' . $seed . ')';
        break;
      case 'pgsql':
        // For PgSQL we'll run an extra query with a integer between
        // 0 and 1 which will be used by the RANDOM() function.
        db_query('select setseed(' . $seed . ')');
        db_query("select random()");
        $formula = 'RANDOM()';
        break;
    }
    if (!empty($formula)) {
      $this->query->add_orderby(NULL, $formula, $this->options['order'], '_' . $this->field);
    }
  }
}

/**
 * Calculate a seed.
 *
 * @param string $view_name
 *   The name of the view.
 * @param string $display_name
 *   The current display name.
 * @param array $options
 *   The options for the random seed handler.
 *
 * @return int
 *   Seed value which is a timestamp.
 */
function views_random_seed_calculate_seed($view_name, $display_name, $options) {
  $time = REQUEST_TIME;
  $seed = NULL;

  if ($options['shared_seed']) {
    $seed_name = $view_name;
  }
  else {
    $seed_name = $view_name . '-' . $display_name;
  }

  // Check if any modules want to generate a seed.
  foreach (module_implements('views_random_seed_calculate') as $module) {
    $module_seed = module_invoke($module, 'views_random_seed_calculate', $seed_name, $view_name, $display_name, $options);
    if (isset($module_seed)) {
      $seed = $module_seed;
    }
  }

  if (!$seed) {
    $seed = _views_random_seed_generate_seed($seed_name, $time);
  }

  // Reset seed or not?
  if (!isset($module_seed) && $options['reset_seed_int'] != 'never') {
    $reset_time = $options['reset_seed_int'] == 'custom' ? $options['reset_seed_custom'] : $options['reset_seed_int'];
    $seed_time = _views_random_seed_get_time($seed_name);
    if (($seed_time + $reset_time) < $time) {
      _views_random_seed_del_seed($seed_name);
      $seed = _views_random_seed_generate_seed($seed_name, $time);
    }
  }

  // Adjust the seed based on UID if different per user.
  if ($options['user_seed_type'] === 'diff_per_user') {
    $seed += _views_random_seed_create_int($GLOBALS['user']->uid);
  }

  // Return seed.
  return $seed;
}

/**
 * Helper function to generate a seed
 *
 * @param string $seed_name
 *   Name of the seed.
 * @param int $time
 *   Current timestamp.
 *
 * @return int seed value.
 */
function _views_random_seed_generate_seed($seed_name, $time) {
  $seed = _views_random_seed_get_seed($seed_name);
  if ($seed == FALSE) {
    $seed = _views_random_seed_create_int($time);
    _views_random_seed_set_seed($seed_name, $seed);
  }

  return $seed;
}

/**
 * Helper function to create a seed based on db_type. MySQL can
 * handle any integer in the RAND() function, Postgres needs
 * an int between 0 and 1.
 *
 * @param int $time
 *   Current timestamp.
 *
 * @return int
 *   Timestamp or int between 0 and 1.
 */
function _views_random_seed_create_int($time) {
  $db_type = db_driver();
  switch ($db_type) {
    case 'mysql':
    case 'mysqli':
      return $time;
      break;
    case 'pgsql':
      $seed = $time / 10000000000;
      return $seed;
      break;
  }
}

/**
 * Save a seed for persisting across requests.
 */
function _views_random_seed_set_seed($seed_name, $value) {
  $seeds = state_get('views_random_seeds', array());
  $seeds[$seed_name] = array(
    'seed' => $value,
    'time' => REQUEST_TIME,
  );
  state_set('views_random_seeds', $seeds);
}

/**
 * Delete a saved seed.
 */
function _views_random_seed_del_seed($seed_name = NULL) {
  $seeds = state_get('views_random_seeds', array());
  if (isset($seeds[$seed_name])) {
    unset($seeds[$seed_name]);
    state_set('views_random_seeds', $seeds);
  }
}

/**
 * Get a seed's value by its name.
 */
function _views_random_seed_get_seed($seed_name) {
  $seeds = state_get('views_random_seeds', array());
  if (isset($seeds[$seed_name])) {
    return $seeds[$seed_name]['seed'];
  }
  return FALSE;
}

/**
 * Get a seed's generation time by its name.
 */
function _views_random_seed_get_time($seed_name) {
  $seeds = state_get('views_random_seeds', array());
  if (isset($seeds[$seed_name])) {
    return $seeds[$seed_name]['time'];
  }
  return FALSE;
}
