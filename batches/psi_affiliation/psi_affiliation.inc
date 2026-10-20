<?php

/**
 * Update PSI author affiliations on one publication.
 */
class change_psi_affiliation extends batch_edit_mods {
    protected $update_dc = false;

    protected function edit_mxds() {
        $csv_rows = isset($this->options['csv_rows']) ? $this->options['csv_rows'] : array();

        if(empty($csv_rows)) return $this->get_mxds_content();

        $edited = false;
        $this->set_dom();
        $this->set_xpath();

        $names = $this->get_xpath()->query('//mods:mods/mods:name[@type="personal"]');

        foreach($names as $name) {
            $author_id = null;
            foreach($name->getElementsByTagNameNS($this->mxds_namespaceURI,'nameIdentifier') as $identifier) {
                if($identifier->getAttribute('type') == 'authorId') {
                    $author_id = $this->normalize_id($identifier->textContent);
                    break;
                }
            }

            if($author_id === null || $author_id === '') continue;

            foreach($csv_rows as $row) {
                if(!isset($row['psi_author_id'])) continue;
                if($this->normalize_id($row['psi_author_id']) !== $author_id) continue;

                // Update the affiliation fields.
                foreach (array('group', 'section', 'department', 'division') as $type) {
                    if(isset($row[$type]) &&
                        $this->set_tagged(
                            $name,
                            'affiliation',
                            $type,
                            $row[$type]
                        )
                    ) $edited = true;
                }

                // https://github.com/Lib4RI/lib4ri_pub_db/blob/7.x/utils/classes.inc#L519-L537
                if (
                    isset($row['org_unit_id']) &&
                    $this->set_tagged(
                        $name,
                        'nameIdentifier',
                        'organizational unit id',
                        $row['org_unit_id']
                    )
                ) $edited = true;

                break;
            }
        }

        return $edited ? $this->get_dom()->saveXML() : $this->get_mxds_content();
    }


    /**
     * Make "psi:1234" and "1234" comparable.
     */
    private function normalize_id($id) {
        $id = trim((string) $id);
        $colon = strrpos($id, ':');

        return trim($colon !== false ? substr($id, $colon + 1) : $id);
    }


    /**
     * Update, create or remove a typed XML element.
     */
    private function set_tagged($parent, $tag, $type, $value) {
        $value = trim((string) $value);
        $node = null;

        foreach ($parent->childNodes as $child) {
            if (
                $child instanceof DOMElement &&
                $child->localName === $tag &&
                $child->namespaceURI === $this->mxds_namespaceURI &&
                $child->getAttribute('type') === $type
            ) {
                $node = $child;
                break;
            }
        }

        // Empty CSV value
        if($value === '') {
            if($node === null) return false;

            $parent->removeChild($node);
            return true;
        }

        if($node !== null) {
            if($node->textContent === $value) return false;

            while($node->firstChild) $node->removeChild($node->firstChild);
            $node->appendChild($this->get_dom()->createTextNode($value));

            return true;
        }

        // Missing element
        $node = $this->get_dom()->createElementNS($this->mxds_namespaceURI,$tag);
        $node->setAttribute('type', $type);

        $node->appendChild($this->get_dom()->createTextNode($value));
        $parent->appendChild($node);

        return true;
    }
}