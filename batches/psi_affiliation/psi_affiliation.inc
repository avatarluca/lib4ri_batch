<?php

require_once dirname(__FILE__) . '/csv_load.inc';

/**
 * Update PSI author affiliations from a CSV file.
 *
 * The CSV contains one row per publication/author combination.
 *
 * Required CSV columns:
 * pub_id;year;psi_author_id;group;section;department;division;org_unit_id
 *
 * Example:
 * 7843;2016;2139;5003 Materialien;;;General Energy ENE;psi-units:533
 * 7843;2016;4220;5003 Materialien;;;General Energy ENE;psi-units:533
 *
 * Command:
 * --command=change_psi_affiliation
 * --command_options="csv=/path/batch_update.csv"
 */
class change_psi_affiliation extends batch_edit_mods {

    protected $update_dc = false;

    protected function edit_mxds() {
        $csv = isset($this->options['csv'])? trim($this->options['csv']) : '';
        if($csv === '' || !is_readable($csv)) return $this->get_mxds_content();

        $pid = trim($this->get_pid());
        $pub_id = $this->normalize_id($pid);
        $rows = array();

        foreach(csv_load::rows($csv) as $row) {
            // if(!isset($row['pub_id']) || !isset($row['psi_author_id'])) continue;

            $row_pub_id = $this->normalize_id($row['pub_id']);
            if($row_pub_id !== $pub_id) continue;
            }

            $author_id = $this->normalize_id(
                $row['psi_author_id']
            );

            if ($author_id === '') {
                continue;
            }

            // Keep the affiliation data for this author.
            $rows[$author_id] = $row;
        }

        // This publication has no matching CSV rows.
        if (empty($rows)) {
            return $this->get_mxds_content();
        }

        // Load MODS and use the namespace declared by the actual XML.
        $this->set_dom();

        $root = $this->get_dom()->documentElement;

        if ($root && !empty($root->namespaceURI)) {
            $this->mxds_namespaceURI = $root->namespaceURI;
        }
        else {
            $this->mxds_namespaceURI =
                'http://www.loc.gov/mods/v3';
        }

        $this->set_xpath();

        $edited = false;

        // Find personal author names directly under the MODS root.
        $names = $this->get_xpath()->query(
            '//mods:mods/mods:name[@type="personal"]'
        );

        foreach ($names as $name) {

            // Find the existing PSI author ID.
            $author_id = '';

            foreach (
                $this->get_direct_elements($name, 'nameIdentifier')
                as $identifier
            ) {
                if (
                    $identifier->getAttribute('type') === 'authorId'
                ) {
                    $author_id = $this->normalize_id(
                        $identifier->textContent
                    );

                    break;
                }
            }

            // Do not create new authors.
            // Only update an author that already exists in MODS.
            if ($author_id === '') {
                continue;
            }

            if (!isset($rows[$author_id])) {
                continue;
            }

            $row = $rows[$author_id];

            // Update the affiliation fields for this author.
            $affiliation_fields = array(
                'group',
                'section',
                'department',
                'division',
            );

            foreach ($affiliation_fields as $type) {

                $value = isset($row[$type])
                    ? $row[$type]
                    : '';

                if (
                    $this->set_tagged(
                        $name,
                        'affiliation',
                        $type,
                        $value
                    )
                ) {
                    $edited = true;
                }
            }

            // Update the organizational unit identifier.
            $org_unit_id = isset($row['org_unit_id'])
                ? $row['org_unit_id']
                : '';

            if (
                $this->set_tagged(
                    $name,
                    'nameIdentifier',
                    'organizational unit id',
                    $org_unit_id
                )
            ) {
                $edited = true;
            }
        }

        if ($edited) {
            return $this->get_dom()->saveXML();
        }

        return $this->get_mxds_content();
    }


    /**
     * Normalize IDs so that 7843 and ir:7843 match.
     */
    private function normalize_id($value) {

        $value = trim((string) $value);

        if ($value === '') {
            return '';
        }

        $colon = strrpos($value, ':');

        if ($colon !== false) {
            $value = substr($value, $colon + 1);
        }

        return trim($value);
    }


    /**
     * Return direct child elements in the current MODS namespace.
     */
    private function get_direct_elements($parent, $tag) {

        $elements = array();

        foreach ($parent->childNodes as $child) {

            if (
                $child instanceof DOMElement &&
                $child->localName === $tag &&
                $child->namespaceURI === $this->mxds_namespaceURI
            ) {
                $elements[] = $child;
            }
        }

        return $elements;
    }


    /**
     * Set, create or remove an element identified by its type attribute.
     *
     * Empty CSV values remove existing elements of that type.
     */
    private function set_tagged($parent, $tag, $type, $value) {

        $value = trim((string) $value);

        $matches = array();

        foreach ($this->get_direct_elements($parent, $tag) as $node) {

            if ($node->getAttribute('type') === $type) {
                $matches[] = $node;
            }
        }

        // An empty CSV value means the field should be absent.
        if ($value === '') {

            foreach ($matches as $node) {
                $node->parentNode->removeChild($node);
            }

            return !empty($matches);
        }

        // Update an existing element.
        if (!empty($matches)) {

            $node = array_shift($matches);
            $edited = false;

            // Remove duplicate elements with the same type.
            foreach ($matches as $duplicate) {
                $duplicate->parentNode->removeChild($duplicate);
                $edited = true;
            }

            if ($node->textContent !== $value) {

                while ($node->firstChild) {
                    $node->removeChild($node->firstChild);
                }

                $node->appendChild(
                    $this->get_dom()->createTextNode($value)
                );

                $edited = true;
            }

            return $edited;
        }

        // Create an element in the correct MODS namespace.
        $prefix = $parent->prefix;

        $qualified_name = ($prefix !== null && $prefix !== '')
            ? $prefix . ':' . $tag
            : $tag;

        $node = $this->get_dom()->createElementNS(
            $this->mxds_namespaceURI,
            $qualified_name
        );

        $node->setAttribute('type', $type);

        $node->appendChild(
            $this->get_dom()->createTextNode($value)
        );

        // In MODS, affiliation should precede role and nameIdentifier.
        if ($tag === 'affiliation') {

            $later_elements = array(
                'role',
                'description',
                'etal',
                'nameIdentifier',
            );

            foreach ($parent->childNodes as $child) {

                if (
                    $child instanceof DOMElement &&
                    $child->namespaceURI === $this->mxds_namespaceURI &&
                    in_array(
                        $child->localName,
                        $later_elements,
                        true
                    )
                ) {
                    $parent->insertBefore($node, $child);

                    return true;
                }
            }
        }

        $parent->appendChild($node);

        return true;
    }
}