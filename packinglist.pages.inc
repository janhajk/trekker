<?php

/**
 * @file
 * Page callback and functions for packing lists.
 */

/**
 * Display a packing list for viewing or altering.
 *
 * @param $wid
 *   The ID of the packing list to display.
 * @return
 *   HTML output for the packing list display and form.
 */
function packinglist_display($wid = NULL, $mode = 'wid') {
  global $user;
  $output = '';

  // $own defines whether the packing list is owned by the current user or not.
  // This affects how many parts of the page are rendered.
  $own = FALSE;

  if ($mode == 'user') {
    // $wid is actually a user account
    $wid = packinglist_get_wid($wid->uid);
  }
  elseif (empty($wid)) {
    // Default to the current user's packing list if no packinglist ID is specified.
    $wid = packinglist_get_wid();
    $own = TRUE;
  }

  if (!$own && $wid == packinglist_get_wid()) {
    $own = TRUE;
  }

  // Attempt to load the packing list.
  $packinglist = packinglist_load($wid);

  // Handle a non-existent packing list.
  if (!$packinglist) {
    // If the requested list was for the current user...
    if ($own) {
      // Display a message letting them know their list is empty.
      drupal_set_title(t('Packing list'));
      drupal_set_message(t("You have not added any gears to your packing list. You can add any gear  to your packing list by clicking the 'Add to packing list' button on the product's page."));

      return t('There are no items on this packing list.');
    }
    else {
      // Otherwise send them to the search form.
      drupal_set_message(t('The packing list you requested could not be found.  Perhaps you can try looking for it through the packing list search form below.'));
      drupal_goto('packinglist/search');
    }
  }

  drupal_set_title($packinglist->title);

  // Add the settings form if the user is viewing his own packing list.
  if ($own) {
    if (!$user->uid) {
      drupal_set_message(filter_xss(t('You must <a href="!login_url">login</a> or <a href="!register_url">register</a> to save your packing list.', array('!login_url' => url('user/login'), '!register_url' => url('user/register')))));
    }

    $collapsed = TRUE;

    $form = drupal_get_form('packinglist_settings_form', $packinglist, $collapsed);
    $output .= drupal_render($form);
  }

  $items = packinglist_get_contents($wid);

  if (empty($items)) {
    return '<p>' . t('There are no items on this packing list.') . '</p>';
  }

  $form = drupal_get_form('packinglist_view_form', $items, $wid, $own);
  $output .= drupal_render($form);

  return $output;
}

/**
 * Builds the user packing list settings form.
 */
function packinglist_settings_form($form, $form_state, $packinglist, $collapsed = FALSE) {
  global $user;

  drupal_add_js(drupal_get_path('module', 'packinglist') . '/packinglist.js');

  $form = array();

  $form['packinglist'] = array(
    '#type' => 'fieldset',
    '#title' => t('Packing list settings'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
  );
  $form['packinglist']['wid'] = array(
    '#type' => 'hidden',
    '#value' => $packinglist->wid,
  );
  $form['packinglist']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $packinglist->title,
    '#required' => TRUE,
  );
  $form['packinglist']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );

  return $form;
}

/**
 * Validation handler for packing list settings form.
 */
function packinglist_settings_form_validate($form, &$form_state) {
  global $user;

  $wid = $form_state['values']['wid'];

  $packinglist = packinglist_load($wid);
  if (!$packinglist) {
    drupal_set_message(t('Could not find the specified packing list.'), 'error');
    return FALSE;
  }
  if ($packinglist->uid != $user->uid && !user_perm('administer packing lists')) {
    drupal_set_message(t('You do not have permission to edit this packing list.'), 'error');
    return FALSE;
  }
}

/**
 * Submission handler for packing list settings form.
 */
function packinglist_settings_form_submit($form, &$form_state) {
  packinglist_update_packinglist($form_state['values']['wid'], $form_state['values']['title']);
  drupal_set_message(t('Your packing list has been updated.'));
}

/**
 * Display a page allowing the customer to view his/her packing list.
 * 
 * TODO: Addapt properties
 */
function packinglist_view_form($form, $form_state, $items, $wid, $own) {
  $form = array();

  $form['items'] = array(
    '#tree' => TRUE,
  );

  // Load each packing list item and add it to the form array.
  foreach ($items as $item) {
    $node = node_load($item->nid);
    $element = array();

    $element['nid'] = array(
      '#type' => 'value',
      '#value' => $node->nid,
    );
    $element['wpid'] = array(
      '#type' => 'value',
      '#value' => $item->wpid,
    );
      
      // TODO: What's this?
    $element['module'] = array(
      '#type' => 'value',
      '#value' => 'uc_product',
    );

    if ($own) {
      $element['remove'] = array('#type' => 'checkbox');
    }

    $item->haveqty = 0;
    if (is_array($item->purchase)) {
      $item->haveqty = count($item->purchase);
    }

    $element['title'] = array(
      '#type' => 'item',
      '#markup' => l(filter_xss($node->title, array()), 'node/' . $node->nid),
    );

    $description = uc_product_get_description($item);
    if ($description) {
      $element['description'] = array(
        '#type' => 'item',
        '#markup' => $description,
      );
    }

    $element['#total'] = $item->price * $item->qty;

    $element['data'] = array(
      '#type' => 'hidden',
      '#value' => serialize($item->data),
    );

    $element['wantqty'] = array(
      '#type' => 'textfield',
      '#default_value' => $item->qty,
      '#size' => 5,
      '#maxlength' => 6,
      '#disabled' => $own ? FALSE : TRUE,
    );
    if (!$own) {
      // Disabled elements do not pass their default value.
      $element['wantqty']['#value'] = $item->qty;
    }
    $element['haveqty'] = array(
      '#type' => 'textfield',
      '#default_value' => $item->haveqty,
      '#size' => 5,
      '#maxlength' => 6,
      '#disabled' => TRUE,
    );
    $element['qty'] = array(
      '#type' => 'textfield',
      '#default_value' => $item->qty - $item->haveqty > 0 ? $item->qty - $item->haveqty : 1,
      '#size' => 5,
      '#maxlength' => 6,
    );

    $form['items'][] = $element;
  }

  $form['wid'] = array(
    '#type' => 'hidden',
    '#value' => $wid,
  );
    
  if ($own) {
    $form['own'] = array(
      '#type' => 'value',
      '#value' => TRUE,
    );
    $form['update'] = array(
      '#type' => 'submit',
      '#value' => t('Update packing list'),
    );
  }

  return $form;
}

/**
 * Theme function for packing list display.
 */
function theme_packinglist_view_form($variables) {
  $form = $variables['form'];
  $rows = array();

  $own = isset($form['own']) ? TRUE : FALSE;

  $header = array(t('Gear'), '', t('Wanted'), t('Have'), t('Qty.'), array(
      'data' => t('Total'),
      'align' => 'right',
    ));

  if ($own) {
    array_unshift($header, t('Remove'));
  }

  foreach (element_children($form['items']) as $i) {
    $nid = $form['items'][$i]['nid']['#value'];
    $desc = drupal_render($form['items'][$i]['title']) . '<br />';
    $desc .= drupal_render($form['items'][$i]['description']);
    $img = uc_product_get_picture($nid, 'uc_cart');

    $row = array(
      empty($img) ? '' : drupal_render($img),
      $desc,
      $form['items'][$i]['wantqty'] ? drupal_render($form['items'][$i]['wantqty']) : '',
      $form['items'][$i]['haveqty'] ? drupal_render($form['items'][$i]['haveqty']) : '',
      $form['items'][$i]['qty'] ? drupal_render($form['items'][$i]['qty']) : '',
      array(
        'data' => uc_currency_format($form['items'][$i]['#total']),
        'nowrap' => 'nowrap',
      ),
    );

    if (isset($form['items'][$i]['remove'])) {
      array_unshift($row, drupal_render($form['items'][$i]['remove']));
    }

    $rows[] = array(
      'data' => $row,
      'valign' => 'top',
    );
  }

  $output = '<div id="packinglist-form-products">' . theme('table', array('header' => $header, 'rows' => $rows)) . '</div>';

  return $output . drupal_render_children($form);
}

/**
 * Validation handler for packing list view form.
 */
function packinglist_view_form_validate($form, &$form_state) {
  // Find packing list item associated with 'add to cart' submitted.
  foreach ($form_state['values'] as $key => $val) {
    if (strpos($key, 'addcart-') === 0) {
      $wpid = intval(substr($key, 8));
    }
  }

  // If adding item to cart, check quantities.
  if (!empty($wpid)) {
    foreach ($form_state['values']['items'] as $key => $item) {
      if ($item['wpid'] == $wpid && $item['qty'] == 0) {
        $item['qty'] = 1;
      }
      if ($item['qty'] && ($item['qty'] + $item['haveqty']) > $item['wantqty']) {
        form_set_error($key, t('You have selected to purchase more items than requested.'));
      }
    }
  }
}

/**
 * Submission handler for packing list view form.
 */
function packinglist_view_form_submit($form, &$form_state) {
  if (isset($form_state['values']['op']) && !empty($form_state['values']['op'])) {
    if ($form_state['values']['op'] == $form_state['values']['update']) {
      packinglist_update_item_object((object) $form_state['values']);

      drupal_set_message(t('Your packing list has been updated.'));

      $form_state['redirect'] = 'packinglist/' . $form_state['values']['wid'];
    }
  }
}

/**
 * Displays the packing list search form.
 */
function packinglist_search_form($form, $form_state, $keywords = '') {
  global $user;
  $form = array();

  // Generate link to 'create or manage your packing list'.
  $path = 'packinglist';
  $query = NULL;

  if (!$user->uid && !variable_get('packinglist_allow_anonymous', FALSE)) {
    $path = 'user';
    $query = 'destination=packinglist';
  }

  $form['packinglist_link'] = array(
    '#value' => '<div>' . l(t('Create or manage your packing list.'), $path, array('query' => $query)) . '</div>',
  );

  $form['search'] = array(
    '#type' => 'fieldset',
  );
  $form['search']['keywords'] = array(
    '#type' => 'textfield',
    '#title' => t('Search keywords'),
    '#description' => t('Enter the keywords to use to search packing list titles and addresses.'),
    '#default_value' => $keywords,
  );
  $form['search']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Search'),
  );

  if (!empty($keywords)) {
    // Check for user, packing list title, or address matches.
    $query = db_select('packinglists', 'w');
    $query->join('users', 'u', 'w.uid = u.uid');
    $query->fields('w', array(
      'wid',
      'title',
    ));
    $query->distinct();
    $query->condition(db_or()
      ->condition('u.name', '%' . $keywords . '%', 'LIKE')
      ->condition('w.title', '%' . $keywords . '%', 'LIKE')
      ->condition('w.address', '%' . $keywords . '%', 'LIKE'));
  }
  else {
    $query = db_select('packinglists', 'w');
    $query->fields('w', array(
      'wid',
      'title',
    ));
  }
  $result = $query->orderBy('w.title')->extend('PagerDefault')->limit(25)->execute();

  $links = array();

  foreach ($result as $packinglist) {
    $links[] = array(
      'title' => filter_xss($packinglist->title, array()),
      'href' => 'packinglist/' . $packinglist->wid,
    );
  }

  if (!empty($links)) {
    $output = theme_links(array(
      'links' => $links,
      'attributes' => array(
        'class' => array('packinglist'),
      ),
      'heading' => NULL,
    ));
  }
  else {
    $output = ' ' . t('No packing lists found.');
  }

  $form['output'] = array(
    '#markup' => filter_xss('<div><h2>' . t('Packing lists:') . '</h2>' . $output . '</div>'),
  );

  return $form;
}

/**
 * Submission handler for packing list search form.
 */
function packinglist_search_form_submit($form, &$form_state) {
  if (empty($form_state['values']['keywords'])) {
    $form_state['redirect'] = 'packinglist/search';
  }
  else {
    $form_state['redirect'] = 'packinglist/search/' . drupal_encode_path($form_state['values']['keywords']);
  }
}
