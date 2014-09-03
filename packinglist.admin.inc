<?php

/**
 * @file
 * Admin page callbacks and forms for packing lists.
 */

/**
 * Build the admin settings form.
 */
function packinglist_admin_settings_form($form, &$form_state) {
  $form = array();
  $form['packinglist_default_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Default packing list title'),
    '#default_value' => variable_get('packinglist_default_title', "%user's packing list"),
    '#description' => t("The default name of a new packing list. The token %user will be replaced by the user's name."),
  );
  return system_settings_form($form);
}

/**
 * View and manage the packing lists on the site.
 */
function packinglist_admin() {
  $rows = array();

  $header = array(
    array(
      'data' => t('User'),
      'field' => 'u.name',
      'sort' => 'asc',
    ),
    array(
      'data' => t('Title'),
      'field' => 'w.title',
    ),
    array('data' => t('Status')),
  );

  // Get a paged list of packing lists from the database.
  $query = db_select('packinglists', 'w');
  $query->leftJoin('users', 'u', 'w.uid = u.uid');
  $query->fields('w', array(
    'wid',
    'uid',
    'title',
  ));
  $query->addField('u', 'name');
  $result = $query->extend('PagerDefault')->limit(25)->execute();

  foreach ($result as $packinglist) {
    // Build the operations array for the packing list.
    $op = array(
      $packinglist->expiration < REQUEST_TIME ? t('Expired') : t('Active'),
      l(t('Delete'), 'admin/store/customers/packinglist/' . $packinglist->wid . '/delete'),
    );

    $rows[] = array(
      $packinglist->name ? l($packinglist->name, 'user/' . $packinglist->uid) : t('Anonymous'),
      l(filter_xss($packinglist->title), 'packinglist/' . $packinglist->wid),
      format_date($packinglist->expiration),
      implode(' | ', $op),
    );
  }

  if (empty($rows)) {
    $rows[] = array(array(
        'data' => t('No packing lists found.'),
        'colspan' => 4,
      ));
  }

  return theme('table', array('header' => $header, 'rows' => $rows)) . theme('pager');
}

/**
 * Confirm the deletion of a packing list.
 */
function packinglist_admin_delete_form($form, $form_state, $packinglist) {
  $form = array();

  $form['packinglist'] = array(
    '#type' => 'value',
    '#value' => $packinglist,
  );

  return confirm_form($form, t('Are you sure you want to delete packing list %title?', array('%title' => $packinglist->title)), 'admin/store/customers/packinglist', NULL, t('Delete'));
}

/**
 * Submission handler for packing list deletion form.
 */
function packinglist_admin_delete_form_submit($form, &$form_state) {
  // Delete the packing list.
  packinglist_delete($form_state['values']['packinglist']->wid);

  // Display a message and redirect back to the admin table.
  drupal_set_message(t('packing list %title deleted.', array('%title' => $form_state['values']['packinglist']->title)));

  $form_state['redirect'] = 'admin/store/customers/packinglist';
}
